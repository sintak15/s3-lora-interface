#!/usr/bin/env python3
"""Build SD-card map tiles for the CYD Node Map.

The firmware reads raw little-endian RGB565 files at:
  /s3-lora/tiles/<z>/<x>/<y>.rgb565

This tool can convert tiles that already exist on disk, extract from .mbtiles
databases, or download from a tile URL template that you are permitted to use
for offline caching.
"""

from __future__ import annotations

import argparse
import math
import sys
import time
import urllib.error
import urllib.request
import sqlite3
from io import BytesIO
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Missing dependency: Pillow. Install with: python -m pip install pillow", file=sys.stderr)
    raise SystemExit(2)


TILE_SIZE = 256


def lon_to_tile_x(lon: float, zoom: int) -> int:
    return int(math.floor((lon + 180.0) / 360.0 * (1 << zoom)))


def lat_to_tile_y(lat: float, zoom: int) -> int:
    lat = max(min(lat, 85.05112878), -85.05112878)
    lat_rad = math.radians(lat)
    return int(math.floor((1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * (1 << zoom)))


def km_to_degrees(lat: float, radius_km: float) -> tuple[float, float]:
    lat_delta = radius_km / 111.32
    lon_scale = max(math.cos(math.radians(lat)), 0.01)
    lon_delta = radius_km / (111.32 * lon_scale)
    return lat_delta, lon_delta


def tile_path(root: Path, z: int, x: int, y: int, suffix: str) -> Path:
    return root / str(z) / str(x) / f"{y}{suffix}"


def output_root(out: Path) -> Path:
    if out.drive and not out.root:
        return Path(out.drive + "\\")
    return out


def parse_zoom_spec(spec: str) -> list[int]:
    zooms: set[int] = set()
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            start_text, end_text = part.split("-", 1)
            start, end = int(start_text), int(end_text)
            if end < start:
                start, end = end, start
            zooms.update(range(start, end + 1))
        else:
            zooms.add(int(part))
    if not zooms:
        raise argparse.ArgumentTypeError("zoom must include at least one level")
    for zoom in zooms:
        if zoom < 0 or zoom > 22:
            raise argparse.ArgumentTypeError("zoom levels must be between 0 and 22")
    return sorted(zooms)


def find_source_tile(source_dir: Path, z: int, x: int, y: int) -> Path | None:
    for suffix in (".png", ".jpg", ".jpeg", ".webp"):
        path = tile_path(source_dir, z, x, y, suffix)
        if path.exists():
            return path
    return None


def load_tile(args: argparse.Namespace, z: int, x: int, y: int) -> Image.Image | None:
    if getattr(args, "_mbtiles_conn", None):
        tms_y = (1 << z) - 1 - y
        cursor = args._mbtiles_conn.cursor()
        cursor.execute("SELECT tile_data FROM tiles WHERE zoom_level=? AND tile_column=? AND tile_row=?", (z, x, tms_y))
        row = cursor.fetchone()
        if row and row[0]:
            return Image.open(BytesIO(row[0]))

    if args.source_dir:
        source = find_source_tile(args.source_dir, z, x, y)
        if source:
            return Image.open(source)

    if not args.tile_url:
        return None

    url = args.tile_url.format(z=z, x=x, y=y)
    request = urllib.request.Request(url, headers={"User-Agent": args.user_agent})
    try:
        with urllib.request.urlopen(request, timeout=args.timeout) as response:
            return Image.open(BytesIO(response.read()))
    except urllib.error.HTTPError as exc:
        print(f"HTTP {exc.code}: {url}", file=sys.stderr)
    except urllib.error.URLError as exc:
        print(f"Download failed: {url}: {exc}", file=sys.stderr)
    return None


def write_rgb565(image: Image.Image, output: Path) -> None:
    image = image.convert("RGB").resize((TILE_SIZE, TILE_SIZE), Image.Resampling.LANCZOS)
    pixels = image.tobytes()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as handle:
        for i in range(0, len(pixels), 3):
            r, g, b = pixels[i], pixels[i + 1], pixels[i + 2]
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            handle.write(bytes((rgb565 & 0xFF, rgb565 >> 8)))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create CYD SD-card offline map tiles.")
    area = parser.add_mutually_exclusive_group(required=False)
    area.add_argument("--center", nargs=2, type=float, metavar=("LAT", "LON"), help="center point for a square map area")
    area.add_argument("--bounds", nargs=4, type=float, metavar=("SOUTH", "WEST", "NORTH", "EAST"), help="map bounds")
    area.add_argument("--mbtiles-all", action="store_true", help="Extract all tiles from an MBTiles file (requires --mbtiles)")
    
    parser.add_argument("--mbtiles", type=Path, help="existing .mbtiles file to extract tiles from")
    parser.add_argument("--radius-km", type=float, default=2.0, help="radius around --center, default 2 km")
    parser.add_argument("--zoom", help="zoom level, comma list, or range. Defaults to 14 (or all for --mbtiles-all); examples: 14, 10-14, 10,12,14")
    parser.add_argument("--out", type=Path, required=True, help="SD card root, or any output folder")
    parser.add_argument("--source-dir", type=Path, help="existing XYZ tile folder containing z/x/y.png or jpg files")
    parser.add_argument("--tile-url", help="legal tile URL template, e.g. https://example/{z}/{x}/{y}.png")
    parser.add_argument("--delay", type=float, default=0.25, help="seconds between downloads, default 0.25")
    parser.add_argument("--timeout", type=float, default=20.0, help="download timeout seconds")
    parser.add_argument("--user-agent", default="s3-lora-interface-offline-map-builder/1.0", help="download user agent")
    parser.add_argument("--overwrite", action="store_true", help="replace existing rgb565 files")
    parser.add_argument("--dry-run", action="store_true", help="print tile counts without writing files")
    
    args = parser.parse_args()
    if not args.center and not args.bounds and not args.mbtiles_all:
        parser.error("Must specify --center, --bounds, or --mbtiles-all")
    if args.mbtiles_all and not args.mbtiles:
        parser.error("--mbtiles-all requires --mbtiles")
    return args


def build_zoom(args: argparse.Namespace, z: int, south: float, west: float, north: float, east: float) -> tuple[int, int, int]:
    min_x = lon_to_tile_x(west, z)
    max_x = lon_to_tile_x(east, z)
    min_y = lat_to_tile_y(north, z)
    max_y = lat_to_tile_y(south, z)
    if max_x < min_x:
        min_x, max_x = max_x, min_x
    if max_y < min_y:
        min_y, max_y = max_y, min_y

    count = (max_x - min_x + 1) * (max_y - min_y + 1)
    tiles_root = output_root(args.out) / "s3-lora" / "tiles"
    print(f"Building z{z} tiles x={min_x}..{max_x}, y={min_y}..{max_y} ({count} tiles)")
    if args.dry_run:
        return 0, 0, count

    made = 0
    skipped = 0
    missing = 0
    for x in range(min_x, max_x + 1):
        for y in range(min_y, max_y + 1):
            output = tile_path(tiles_root, z, x, y, ".rgb565")
            if output.exists() and not args.overwrite:
                skipped += 1
                continue
            image = load_tile(args, z, x, y)
            if image is None:
                missing += 1
                continue
            write_rgb565(image, output)
            made += 1
            print(f"wrote {output}")
            if args.tile_url and args.delay > 0:
                time.sleep(args.delay)

    return made, skipped, missing


def main() -> int:
    args = parse_args()
    
    if args.zoom is None:
        if args.mbtiles_all:
            zooms = None
        else:
            zooms = parse_zoom_spec("14")
    else:
        zooms = parse_zoom_spec(args.zoom)

    tiles_root = output_root(args.out) / "s3-lora" / "tiles"
    print(f"Output: {tiles_root}")
    if args.tile_url and "tile.openstreetmap.org" in args.tile_url:
        print("Warning: tile.openstreetmap.org does not permit bulk/offline tile downloading.", file=sys.stderr)

    if args.mbtiles:
        args._mbtiles_conn = sqlite3.connect(args.mbtiles)
    else:
        args._mbtiles_conn = None

    total_made = 0
    total_skipped = 0
    total_missing = 0

    if args.mbtiles_all:
        cursor = args._mbtiles_conn.cursor()
        
        # Build query depending on zoom restriction
        if zooms is not None:
            placeholders = ",".join("?" for _ in zooms)
            cursor.execute(f"SELECT COUNT(*) FROM tiles WHERE zoom_level IN ({placeholders})", list(zooms))
        else:
            cursor.execute("SELECT COUNT(*) FROM tiles")
            
        count = cursor.fetchone()[0]
        print(f"Building from MBTiles ({count} tiles)")
        if args.dry_run:
            approx_bytes = count * TILE_SIZE * TILE_SIZE * 2
            print(f"Dry run. tiles={count} raw_rgb565={approx_bytes / (1024 * 1024 * 1024):.2f} GiB")
            return 0

        if zooms is not None:
            placeholders = ",".join("?" for _ in zooms)
            cursor.execute(f"SELECT zoom_level, tile_column, tile_row, tile_data FROM tiles WHERE zoom_level IN ({placeholders})", list(zooms))
        else:
            cursor.execute("SELECT zoom_level, tile_column, tile_row, tile_data FROM tiles")
            
        for z, x, tms_y, data in cursor:
            y = (1 << z) - 1 - tms_y
            output = tile_path(tiles_root, z, x, y, ".rgb565")
            if output.exists() and not args.overwrite:
                total_skipped += 1
                continue
                
            try:
                image = Image.open(BytesIO(data))
                write_rgb565(image, output)
                total_made += 1
                if total_made % 100 == 0:
                    print(f"wrote {total_made} tiles...", end="\r")
            except Exception as e:
                magic = data[:8].hex() if data else "empty"
                print(f"\nFailed to convert z={z} x={x} y={y}: {e} (First bytes: {magic})", file=sys.stderr)
                total_missing += 1
        print()
    else:
        if args.center:
            center_lat, center_lon = args.center
            lat_delta, lon_delta = km_to_degrees(center_lat, args.radius_km)
            south, west, north, east = center_lat - lat_delta, center_lon - lon_delta, center_lat + lat_delta, center_lon + lon_delta
        else:
            south, west, north, east = args.bounds

        for zoom in zooms:
            made, skipped, missing = build_zoom(args, zoom, south, west, north, east)
            total_made += made
            total_skipped += skipped
            total_missing += missing

    if args.dry_run:
        approx_bytes = total_missing * TILE_SIZE * TILE_SIZE * 2
        print(f"Dry run. tiles={total_missing} raw_rgb565={approx_bytes / (1024 * 1024 * 1024):.2f} GiB")
        return 0

    print(f"Done. wrote={total_made} skipped={total_skipped} missing={total_missing}")
    return 0 if total_missing == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
