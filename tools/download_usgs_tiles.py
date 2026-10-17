#!/usr/bin/env python3
"""Build a resumable offline map pack centered on St. Paul, directly on an SD card.

Requires requests, Pillow, numpy and pyshp. Uses public USGS Topo tiles and Census
state boundaries. Firmware supports zoom 10-14. An interrupted job skips complete
tiles on restart; progress is saved on the PC and a pack description on the card.
"""
from __future__ import annotations
import argparse
import concurrent.futures as futures
import ctypes
from collections import Counter
from io import BytesIO
import json
import math
import os
from pathlib import Path
import shutil
import sys
import threading
import time
import zlib
import zipfile

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import requests

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "build/mapdeps"))
import shapefile

TILE_BYTES = 131072
TILE_SERVICE = "https://basemap.nationalmap.gov/arcgis/rest/services/USGSTopo/MapServer"

def tile_xy(lon, lat, zoom):
    n = 2**zoom
    return (lon + 180) / 360 * n, (1 - math.asinh(math.tan(math.radians(lat))) / math.pi) / 2 * n

def distance_km(lat1, lon1, lat2, lon2):
    a, b = math.radians(lat1), math.radians(lat2)
    h = math.sin((b-a)/2)**2 + math.cos(a)*math.cos(b)*math.sin(math.radians(lon2-lon1)/2)**2
    return 6371.0088 * 2 * math.asin(min(1, math.sqrt(h)))

def read_boundaries(archive):
    with zipfile.ZipFile(archive) as packed:
        shp = next(n for n in packed.namelist() if n.endswith(".shp"))
        stem = shp[:-4]
        reader = shapefile.Reader(shp=BytesIO(packed.read(shp)), shx=BytesIO(packed.read(stem + ".shx")), dbf=BytesIO(packed.read(stem + ".dbf")))
        polygons = []
        for record in reader.iterShapeRecords():
            if record.record.as_dict()["STUSPS"] in {"AK", "HI", "PR", "VI", "GU", "MP", "AS"}:
                continue
            points = record.shape.points
            parts = list(record.shape.parts) + [len(points)]
            polygons.extend(points[parts[i]:parts[i+1]] for i in range(len(parts)-1))
        return polygons

def plan_tiles(boundaries, center, budget_bytes):
    polygons = read_boundaries(boundaries)
    selected, close = [], []
    # Full contiguous-U.S. overview; progressively closer detail as zoom increases.
    radii = {10: float("inf"), 11: float("inf"), 12: 1500, 13: 1000}
    for zoom in range(10, 15):
        x0, y0 = map(math.floor, tile_xy(-125, 50, zoom))
        x1, y1 = map(math.floor, tile_xy(-66, 24, zoom))
        mask = Image.new("L", (x1-x0+1, y1-y0+1))
        draw = ImageDraw.Draw(mask)
        for ring in polygons:
            draw.polygon([(tile_xy(lon, lat, zoom)[0]-x0, tile_xy(lon, lat, zoom)[1]-y0) for lon, lat in ring], fill=255)
        # Include boundary tiles and a one-tile margin for seamless edge viewports.
        mask = np.asarray(mask.filter(ImageFilter.MaxFilter(3)))
        ys, xs = np.nonzero(mask)
        n = 2**zoom
        for local_x, local_y in zip(xs.tolist(), ys.tolist()):
            x, y = local_x+x0, local_y+y0
            lon = (x+0.5)/n*360-180
            lat = math.degrees(math.atan(math.sinh(math.pi*(1-2*(y+0.5)/n))))
            distance = distance_km(*center, lat, lon)
            row = (distance, zoom, x, y)
            if zoom == 14:
                close.append(row)
            elif distance <= radii[zoom]:
                selected.append(row)
    slots = int(budget_bytes) // TILE_BYTES
    if len(selected) >= slots:
        raise ValueError("budget too small for the overview profile")
    close.sort()
    selected.extend(close[:slots-len(selected)])
    selected.sort()  # All available zooms near St. Paul arrive before distant coverage.
    return selected

def rgb565_bytes(image):
    rgb = np.asarray(image.convert("RGB"), dtype=np.uint16)
    packed = ((rgb[:,:,0] & 0xf8) << 8) | ((rgb[:,:,1] & 0xfc) << 3) | (rgb[:,:,2] >> 3)
    return packed.astype("<u2").tobytes()

class Downloader:
    def __init__(self, root, rate, reserve):
        self.root, self.interval, self.reserve = root, 1/rate, reserve
        self.local, self.lock, self.next_request = threading.local(), threading.Lock(), 0.0

    def fetch(self, row):
        _, z, x, y = row
        target = self.root / str(z) / str(x) / f"{y}.rgb565"
        if target.is_file() and target.stat().st_size == TILE_BYTES:
            return "skipped", None
        if not hasattr(self.local, "session"):
            self.local.session = requests.Session()
            self.local.session.headers["User-Agent"] = "s3-lora-interface-offline-map-builder/1.0"
        error = None
        for attempt in range(5):
            try:
                with self.lock:
                    delay = max(0, self.next_request - time.monotonic())
                    self.next_request = max(self.next_request, time.monotonic()) + self.interval
                if delay:
                    time.sleep(delay)
                response = self.local.session.get(f"{TILE_SERVICE}/tile/{z}/{y}/{x}", timeout=(10, 40))
                if response.status_code in (429, 503):
                    time.sleep(min(120, max(2**attempt, float(response.headers.get("Retry-After", "5")))))
                response.raise_for_status()
                with Image.open(BytesIO(response.content)) as image:
                    if image.size != (256, 256):
                        raise ValueError(f"unexpected tile dimensions {image.size}")
                    data = rgb565_bytes(image)
                if len(data) != TILE_BYTES:
                    raise ValueError("incorrect converted tile size")
                if shutil.disk_usage(self.root).free < self.reserve + TILE_BYTES * 8:
                    raise OSError("reserved card space reached")
                target.parent.mkdir(parents=True, exist_ok=True)
                temporary = target.with_suffix(".download-part")
                with temporary.open("wb") as stream:
                    stream.write(data)
                    stream.flush()
                    os.fsync(stream.fileno())
                if temporary.stat().st_size != TILE_BYTES or zlib.crc32(temporary.read_bytes()) != zlib.crc32(data):
                    raise OSError("SD tile readback mismatch")
                temporary.replace(target)
                return "downloaded", None
            except Exception as exc:
                error = str(exc)
                if isinstance(exc, OSError) and not isinstance(exc, requests.RequestException):
                    raise
                time.sleep(min(30, 2**attempt))
        return "failed", {"z": z, "x": x, "y": y, "error": error}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, required=True, help="SD card root")
    parser.add_argument("--boundaries", type=Path, required=True, help="Census cb_2024_us_state_500k.zip")
    parser.add_argument("--work", type=Path, required=True, help="PC-side plan/progress directory")
    parser.add_argument("--center", type=float, nargs=2, default=(44.9537, -93.09), metavar=("LAT", "LON"))
    parser.add_argument("--budget-gib", type=float, default=110)
    parser.add_argument("--reserve-gib", type=float, default=8)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--rate", type=float, default=4, help="maximum total HTTP requests per second")
    parser.add_argument("--limit", type=int, help="download only the nearest N tiles for testing")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.workers < 1 or args.workers > 8 or not 0 < args.rate <= 8 or args.budget_gib <= 0 or args.reserve_gib < 0:
        parser.error("invalid worker, rate or storage limits")
    if not args.destination.is_dir():
        parser.error("destination is not mounted")
    args.work.mkdir(parents=True, exist_ok=True)
    plan = plan_tiles(args.boundaries, args.center, args.budget_gib * 1024**3)
    counts = Counter(row[1] for row in plan)
    description = {"source": TILE_SERVICE, "credit": "U.S. Geological Survey, The National Map",
                   "center": args.center, "centerName": "St. Paul, Minnesota", "tiles": len(plan),
                   "bytes": len(plan)*TILE_BYTES, "zoomCounts": dict(counts),
                   "farthestTileCenterKmByZoom": {z: round(max(r[0] for r in plan if r[1] == z)) for z in counts},
                   "coverage": "Contiguous United States; Census 2024 state boundary mask with one tile margin"}
    (args.work / "plan.json").write_text(json.dumps(description, indent=2), encoding="utf-8")
    print(json.dumps(description), flush=True)
    if args.dry_run:
        return 0
    if (args.work / "STOP").exists():
        raise RuntimeError("STOP file exists; remove it before resuming this job")
    if args.limit:
        plan = plan[:args.limit]
    tile_root = args.destination / "s3-lora/tiles"
    tile_root.mkdir(parents=True, exist_ok=True)
    (tile_root.parent / "map-pack.json").write_text(json.dumps(description, indent=2), encoding="utf-8")
    downloader = Downloader(tile_root, args.rate, args.reserve_gib * 1024**3)
    started, last_report = time.monotonic(), 0
    counts, consecutive_failures = Counter(), 0
    stopping = False
    def report(state):
        elapsed = time.monotonic()-started
        done = sum(counts.values())
        data = {"state": state, "total": len(plan), **dict(counts), "completed": done,
                "GiBDownloadedThisRun": round(counts["downloaded"]/8192, 3), "seconds": round(elapsed),
                "estimatedHoursRemaining": round(elapsed/max(done,1)*(len(plan)-done)/3600, 2)}
        temporary = args.work / "progress.tmp"
        temporary.write_text(json.dumps(data, indent=2), encoding="utf-8")
        temporary.replace(args.work / "progress.json")
        print(json.dumps(data), flush=True)
    iterator = iter(plan)
    # Keep only this process's work session awake; leave global power settings alone.
    if sys.platform == "win32":
        ctypes.windll.kernel32.SetThreadExecutionState(0x80000001)
    try:
        with futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
            pending = {pool.submit(downloader.fetch, row) for row in [next(iterator, None) for _ in range(args.workers)] if row is not None}
            while pending:
                if not stopping and (args.work / "STOP").exists():
                    stopping = True
                    report("stopping")
                finished, pending = futures.wait(pending, timeout=1, return_when=futures.FIRST_COMPLETED)
                for task in finished:
                    outcome, error = task.result()
                    counts[outcome] += 1
                    consecutive_failures = consecutive_failures+1 if error else 0
                    if error:
                        with (args.work / "failures.jsonl").open("a", encoding="utf-8") as log:
                            log.write(json.dumps(error)+"\n")
                    if consecutive_failures >= 12:
                        raise RuntimeError("12 consecutive tiles failed; service may be unavailable")
                    row = None if stopping else next(iterator, None)
                    if row is not None:
                        pending.add(pool.submit(downloader.fetch, row))
                if sum(counts.values()) <= 4 or time.monotonic()-last_report >= 15:
                    report("stopping" if stopping else "downloading")
                    last_report = time.monotonic()
        report("stopped" if stopping else ("complete" if not counts["failed"] else "complete-with-failures"))
    except BaseException:
        report("stopped")
        raise
    finally:
        if sys.platform == "win32":
            ctypes.windll.kernel32.SetThreadExecutionState(0x80000000)
    return 0 if not counts["failed"] else 1

if __name__ == "__main__":
    raise SystemExit(main())
