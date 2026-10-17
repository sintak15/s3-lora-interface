#!/usr/bin/env python3
"""Copy files read-only from the installed CYD SD card to a mounted card reader.

Requires firmware v0.1.85 and pyserial. Each USB chunk, complete source file,
and destination readback is checked with CRC32. No formatting or source writes.
"""
from __future__ import annotations
import argparse
import base64
import json
import os
import re
import secrets
import shutil
import sys
import time
import zlib
from pathlib import Path, PurePosixPath

class CopyError(RuntimeError):
    pass

def destination_path(root, source):
    path = PurePosixPath(source)
    if not source.startswith("/") or "\\" in source or any(p in (".", "..") or ":" in p or any(ord(c) < 32 for c in p) for p in source.split("/") if p):
        raise CopyError(f"unsafe source path: {source}")
    target = root.joinpath(*path.parts[1:])
    if not target.resolve().is_relative_to(root.resolve()):
        raise CopyError(f"path escapes destination: {source}")
    return target

def crc_file(path):
    crc = 0
    with path.open("rb") as stream:
        while data := stream.read(1024 * 1024):
            crc = zlib.crc32(data, crc)
    return f"{crc:08x}"

class SdPort:
    def __init__(self, port, timeout=20):
        self.port, self.timeout, self.pending = port, timeout, b""

    def command(self, text, token, expected, entries=None):
        self.port.write((text + "\n").encode("ascii"))
        self.port.flush()
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            self.pending += self.port.read_until(b"\n")
            if b"\n" not in self.pending:
                if len(self.pending) > 65536:
                    raise CopyError("overlong USB response")
                continue
            raw, self.pending = self.pending.split(b"\n", 1)
            line = raw.decode("ascii", errors="replace").strip()
            prefix = f"[SDUSB] {token} "
            if not line.startswith(prefix):
                continue
            status, _, detail = line[len(prefix):].partition(" ")
            if status == "ENTRY" and entries is not None:
                kind, size, encoded = detail.split(" ", 2)
                name = base64.b64decode(encoded, validate=True).decode("utf-8")
                entries.append({"kind": kind, "size": int(size), "path": name})
                continue
            if status == "ERROR":
                raise CopyError(detail)
            if status not in expected:
                raise CopyError(f"unexpected reply: {status}")
            return status, detail
        raise CopyError(f"USB timeout: {text.split()[0]}")

    def connect(self):
        _, detail = self.command("sd-hello", "00000000", {"HELLO"})
        if "protocol=1" not in detail or "readonly=1" not in detail:
            raise CopyError("firmware does not support read-only SD export")
        self.command("sd-close 00000000", "00000000", {"CLOSED"})

    def list_directory(self, path):
        encoded = base64.b64encode(path.encode()).decode()
        cursor, entries = 0, []
        while True:
            token = f"{secrets.randbelow(0xffffffff) + 1:08x}"
            _, detail = self.command(f"sd-list {token} {encoded} {cursor}", token, {"LIST"}, entries)
            next_cursor, more = map(int, detail.split())
            if not more:
                break
            if next_cursor <= cursor:
                raise CopyError("directory cursor did not advance")
            cursor = next_cursor
        return entries

    def copy_file(self, source, target):
        token = f"{secrets.randbelow(0xffffffff) + 1:08x}"
        encoded = base64.b64encode(source.encode()).decode()
        _, detail = self.command(f"sd-open {token} {encoded} 0", token, {"OPEN"})
        size = int(detail)
        temporary = target.with_name(target.name + ".usbcopy-part")
        target.parent.mkdir(parents=True, exist_ok=True)
        offset, crc = 0, 0
        try:
            with temporary.open("wb") as stream:
                while offset < size:
                    last_error = None
                    for attempt in range(3):
                        try:
                            _, detail = self.command(f"sd-read {token} {offset}", token, {"DATA"})
                            actual_offset, expected_crc, encoded = detail.split(" ", 2)
                            data = base64.b64decode(encoded, validate=True)
                            if int(actual_offset) != offset or not 0 < len(data) <= min(24576, size - offset) or f"{zlib.crc32(data):08x}" != expected_crc:
                                raise CopyError("USB chunk checksum or offset mismatch")
                            break
                        except (CopyError, ValueError) as exc:
                            last_error = exc
                    else:
                        raise CopyError(f"failed after 3 reads: {last_error}")
                    stream.write(data)
                    crc = zlib.crc32(data, crc)
                    offset += len(data)
                _, expected_crc = self.command(f"sd-read {token} {offset}", token, {"EOF"})
                if f"{crc:08x}" != expected_crc:
                    raise CopyError("complete source file checksum mismatch")
                stream.flush()
                os.fsync(stream.fileno())
            if temporary.stat().st_size != size or crc_file(temporary) != expected_crc:
                raise CopyError("destination readback checksum mismatch")
            # Preserve any different pre-existing destination file for review.
            if target.exists() and crc_file(target) != expected_crc:
                backup = target.with_name(target.name + ".before-usb-copy")
                if backup.exists():
                    raise CopyError(f"destination backup already exists: {backup}")
                target.rename(backup)
            temporary.replace(target)
            return {"path": source, "size": size, "crc32": expected_crc}
        finally:
            try:
                self.command(f"sd-close {token}", token, {"CLOSED"})
            except Exception:
                pass

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--destination", type=Path, required=True, help="mounted replacement SD card root")
    parser.add_argument("--manifest", type=Path, required=True, help="PC-side verified-file journal used for resume")
    parser.add_argument("--progress", type=Path, required=True)
    parser.add_argument("--limit", type=int, help="copy only the first N files for a smoke test")
    parser.add_argument("--exclude-path", action="append", default=[], help="omit this source directory and its contents (repeatable)")
    args = parser.parse_args()
    root = args.destination.resolve()
    if not root.is_dir() or root.parent != root:
        parser.error("destination must be an existing mounted drive root")
    if args.limit is not None and args.limit <= 0:
        parser.error("--limit must be positive")
    import serial
    port = serial.Serial()
    port.port, port.baudrate, port.timeout, port.write_timeout = args.port, 115200, 0.5, 10
    port.dtr, port.rts = False, False
    verified = {}
    if args.manifest.exists():
        for line in args.manifest.read_text(encoding="utf-8").splitlines():
            try:
                row = json.loads(line)
                verified[row["path"]] = row
            except (ValueError, KeyError):
                continue
    started, copied, skipped, total_bytes = time.monotonic(), 0, 0, 0
    last_progress = 0
    def progress(state="copying", current="", error=None):
        elapsed = time.monotonic() - started
        report = {"state": state, "destination": str(root), "filesCopied": copied, "filesSkipped": skipped,
                  "bytesCopied": total_bytes, "seconds": round(elapsed, 1), "MiBPerSecond": round(total_bytes / max(elapsed, 0.001) / 1024**2, 3), "current": current}
        if error:
            report["error"] = str(error)
        temporary = args.progress.with_suffix(".tmp")
        temporary.write_text(json.dumps(report, indent=2), encoding="utf-8")
        temporary.replace(args.progress)
        print(json.dumps(report), flush=True)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    try:
        port.open()
        time.sleep(0.3)
        client = SdPort(port)
        client.connect()
        pending = ["/"]
        with args.manifest.open("a", encoding="utf-8") as journal:
            while pending:
                folder = pending.pop()
                entries = client.list_directory(folder)
                for entry in entries:
                    source = entry["path"]
                    if any(source == excluded.rstrip("/") or source.startswith(excluded.rstrip("/") + "/") for excluded in args.exclude_path):
                        continue
                    target = destination_path(root, source)
                    # Windows manages these directories on the replacement card itself.
                    if source.split("/")[1].lower() in ("system volume information", "$recycle.bin"):
                        continue
                    if entry["kind"] == "D":
                        target.mkdir(parents=True, exist_ok=True)
                        pending.append(source)
                        continue
                    record = verified.get(source)
                    # Only immutable map tiles can be resumed from a prior verified copy.
                    # Logs and snapshots are copied again because the running device updates them.
                    if source.endswith(".rgb565") and record and record["size"] == entry["size"] and target.is_file() and target.stat().st_size == record["size"] and crc_file(target) == record["crc32"]:
                        skipped += 1
                    else:
                        if shutil.disk_usage(root).free < entry["size"] + 1024**2:
                            raise CopyError("insufficient space on destination card")
                        record = client.copy_file(source, target)
                        journal.write(json.dumps(record) + "\n")
                        journal.flush()
                        os.fsync(journal.fileno())
                        copied += 1
                        total_bytes += record["size"]
                    if copied + skipped <= 3 or time.monotonic() - last_progress >= 10:
                        progress(current=source)
                        last_progress = time.monotonic()
                    if args.limit and copied + skipped >= args.limit:
                        progress("sample-complete", source)
                        return 0
        progress("complete")
        return 0
    except BaseException as exc:
        progress("stopped", error=exc)
        raise
    finally:
        port.close()

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (CopyError, OSError, ValueError) as exc:
        print(f"Copy stopped: {exc}. Rerun with the same manifest to resume verified tiles.", file=sys.stderr)
        raise SystemExit(1)
