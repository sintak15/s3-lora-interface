#!/usr/bin/env python3
"""Upload a prepared RGB565 XYZ tile tree through the CYD USB port.

Requires pyserial. Firmware v0.1.85 or later; the radio UART is not supported.
Existing tiles with matching size/CRC are skipped on the device, allowing resume.
"""
from __future__ import annotations

import argparse
import base64
import json
import re
import secrets
import sys
import time
import zlib
from pathlib import Path

TILE_BYTES = 131072
CHUNK_BYTES = 3072
REPLY = re.compile(r"^\[TILEUSB\] ([0-9a-f]{8}) ([A-Z]+)(?: (.*))?$")


class UploadError(RuntimeError):
    pass


class TilePort:
    def __init__(self, port, timeout=15.0, log=None):
        self.port, self.timeout, self.log = port, timeout, log
        self.pending = b""

    def command(self, text, token, expected):
        self.port.write((text + "\n").encode("ascii"))
        self.port.flush()
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            self.pending += self.port.readline()
            if b"\n" not in self.pending:
                if len(self.pending) > 16384:
                    raise UploadError("unterminated USB response")
                continue
            raw, self.pending = self.pending.split(b"\n", 1)
            line = raw.decode("ascii", errors="replace").strip()
            if self.log:
                self.log.write(line + "\n")
                self.log.flush()
            match = REPLY.fullmatch(line)
            if not match or match[1] != token:
                continue  # Normal device diagnostics share USB; only consume our replies.
            status, detail = match[2], match[3] or ""
            if status == "ERROR":
                raise UploadError(detail)
            if status not in expected:
                raise UploadError(f"unexpected reply: {line}")
            return status, detail
        raise UploadError(f"timed out waiting for {', '.join(expected)}")

    def connect(self):
        _, details = self.command("tile-hello", "00000000", {"HELLO"})
        fields = dict(part.split("=", 1) for part in details.split() if "=" in part)
        if fields.get("protocol") != "1" or fields.get("bytes") != str(TILE_BYTES) or fields.get("chunk") != str(CHUNK_BYTES):
            raise UploadError("incompatible tile upload protocol")
        # Explicitly release a partial upload left by a disconnected uploader.
        self.command("tile-abort 00000000", "00000000", {"ABORTED"})

    def upload(self, path, coordinates, overwrite=False):
        data = path.read_bytes()
        if len(data) != TILE_BYTES:
            raise UploadError(f"wrong tile size: {path} ({len(data)} bytes)")
        z, x, y = coordinates
        token = f"{secrets.randbelow(0xffffffff) + 1:08x}"
        crc = f"{zlib.crc32(data):08x}"
        try:
            status, detail = self.command(f"tile-begin {token} {z} {x} {y} {crc} {int(overwrite)}", token, {"READY", "SKIP"})
            if status == "SKIP":
                if detail != crc:
                    raise UploadError("skip checksum mismatch")
                return "skipped"
            for offset in range(0, len(data), CHUNK_BYTES):
                chunk = data[offset:offset + CHUNK_BYTES]
                encoded = base64.b64encode(chunk).decode("ascii")
                _, detail = self.command(f"tile-data {token} {offset} {encoded}", token, {"ACK"})
                if detail != str(offset + len(chunk)):
                    raise UploadError("chunk acknowledgement offset mismatch")
            _, detail = self.command(f"tile-end {token}", token, {"DONE"})
            if detail != crc:
                raise UploadError("final checksum mismatch")
            return "uploaded"
        except BaseException:
            try:
                self.command(f"tile-abort {token}", token, {"ABORTED"})
            except Exception:
                pass  # The device also expires incomplete sessions after 30 seconds.
            raise


def tile_coordinates(path, root):
    parts = path.relative_to(root).parts
    if len(parts) != 3 or path.suffix != ".rgb565":
        raise UploadError(f"expected z/x/y.rgb565 beneath --tiles: {path}")
    try:
        z, x, y = int(parts[0]), int(parts[1]), int(path.stem)
    except ValueError as exc:
        raise UploadError(f"invalid tile coordinates: {path}") from exc
    if not (10 <= z <= 14 and 0 <= x < 2**z and 0 <= y < 2**z):
        raise UploadError(f"tile outside firmware zoom/coordinate limits: {path}")
    return z, x, y


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="CYD USB COM port, e.g. COM12 (required unless --dry-run)")
    parser.add_argument("--tiles", type=Path, required=True, help="tile root containing z/x/y.rgb565")
    parser.add_argument("--overwrite", action="store_true", help="replace different existing tiles after CRC/readback verification")
    parser.add_argument("--dry-run", action="store_true", help="validate paths/sizes and show storage without connecting")
    parser.add_argument("--limit", type=int, help="upload at most this many tiles")
    parser.add_argument("--log", type=Path, help="save USB diagnostics and protocol replies")
    args = parser.parse_args()
    if args.limit is not None and args.limit <= 0:
        parser.error("--limit must be positive")
    if not args.tiles.is_dir():
        parser.error("--tiles must be an existing directory")
    paths = sorted(args.tiles.glob("*/*/*.rgb565"), key=lambda p: tile_coordinates(p, args.tiles))
    if args.limit:
        paths = paths[:args.limit]
    if not paths:
        parser.error("no RGB565 tiles found")
    # Reject malformed files before writing anything on the device.
    tiles = [(path, tile_coordinates(path, args.tiles)) for path in paths]
    for path, _ in tiles:
        if path.stat().st_size != TILE_BYTES:
            raise UploadError(f"wrong tile size: {path}")
    print(f"{len(tiles):,} tiles, {len(tiles) * TILE_BYTES / 1024**3:.3f} GiB RGB565", flush=True)
    if args.dry_run:
        return 0
    if not args.port:
        parser.error("--port is required")
    import serial
    started = time.monotonic()
    counts = {"uploaded": 0, "skipped": 0}
    log = args.log.open("a", encoding="utf-8") if args.log else None
    port = serial.Serial()
    # Set control lines before opening to avoid resetting a running CYD.
    port.port, port.baudrate, port.timeout, port.write_timeout = args.port, 115200, 0.25, 10
    port.dtr, port.rts = False, False
    try:
        port.open()
        time.sleep(0.3)
        client = TilePort(port, log=log)
        client.connect()
        for index, (path, xyz) in enumerate(tiles, 1):
            result = client.upload(path, xyz, overwrite=args.overwrite)
            counts[result] += 1
            if index <= 3 or index % 25 == 0 or index == len(tiles):
                elapsed = time.monotonic() - started
                eta = elapsed / index * (len(tiles) - index)
                print(f"{index:,}/{len(tiles):,}: uploaded={counts['uploaded']:,} skipped={counts['skipped']:,} elapsed={elapsed:.0f}s remaining~{eta / 60:.1f}min", flush=True)
    finally:
        port.close()
        if log:
            log.close()
    print(json.dumps({**counts, "seconds": round(time.monotonic() - started, 2)}), flush=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (UploadError, OSError) as exc:
        print(f"Upload stopped: {exc}. Rerun the same command to resume.", file=sys.stderr)
        raise SystemExit(1)
