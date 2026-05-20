#!/usr/bin/env python3
import argparse
import math
import time
import urllib.request
from pathlib import Path
from tempfile import NamedTemporaryFile

from PIL import Image


def lon_to_tile_x(lon, zoom):
    return int(math.floor((lon + 180.0) / 360.0 * (1 << zoom)))


def lat_to_tile_y(lat, zoom):
    lat_rad = math.radians(lat)
    n = 1 << zoom
    return int(math.floor((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n))


def clamp_tile(value, zoom):
    return max(0, min((1 << zoom) - 1, value))


def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def write_rgb565_tile(image_path, output_path):
    with Image.open(image_path) as image:
        image = image.convert("RGB").resize((256, 256), Image.Resampling.LANCZOS)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with output_path.open("wb") as out:
            for r, g, b in image.getdata():
                value = rgb888_to_rgb565(r, g, b)
                out.write(bytes((value & 0xFF, value >> 8)))


def download_tile(url_template, zoom, x, y, user_agent, delay_s):
    url = url_template.format(z=zoom, x=x, y=y)
    request = urllib.request.Request(url, headers={"User-Agent": user_agent})
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read()
    if delay_s > 0:
        time.sleep(delay_s)
    tmp = NamedTemporaryFile(delete=False, suffix=Path(url).suffix or ".tile")
    tmp.write(data)
    tmp.close()
    return Path(tmp.name)


def find_source_tile(input_dir, zoom, x, y):
    base = input_dir / str(zoom) / str(x) / str(y)
    for suffix in (".png", ".jpg", ".jpeg", ".webp"):
        candidate = base.with_suffix(suffix)
        if candidate.exists():
            return candidate
    return None


def parse_zooms(text):
    zooms = set()
    for part in text.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            start, end = part.split("-", 1)
            zooms.update(range(int(start), int(end) + 1))
        else:
            zooms.add(int(part))
    return sorted(zooms)


def main():
    parser = argparse.ArgumentParser(description="Prepare RGB565 SD map tiles for s3-lora-interface.")
    parser.add_argument("--bbox", required=True, help="west,south,east,north in decimal degrees")
    parser.add_argument("--zooms", default="10-14", help="Zooms such as 10-14 or 10,12,14")
    parser.add_argument("--output", default="sdcard/s3-lora/tiles", type=Path, help="Output tiles directory")
    parser.add_argument("--input", type=Path, help="Existing slippy tile directory with z/x/y.png files")
    parser.add_argument("--url-template", help="Download template, e.g. https://server/{z}/{x}/{y}.png")
    parser.add_argument("--user-agent", default="s3-lora-interface tile prep", help="User-Agent for downloads")
    parser.add_argument("--delay", default=0.25, type=float, help="Delay between downloads in seconds")
    parser.add_argument("--force", action="store_true", help="Regenerate existing .rgb565 files")
    args = parser.parse_args()

    if not args.input and not args.url_template:
        raise SystemExit("Provide --input for local tiles or --url-template for downloading tiles.")

    west, south, east, north = [float(value.strip()) for value in args.bbox.split(",")]
    zooms = parse_zooms(args.zooms)
    total = 0
    written = 0
    skipped = 0
    missing = 0

    for zoom in zooms:
        x0 = clamp_tile(lon_to_tile_x(west, zoom), zoom)
        x1 = clamp_tile(lon_to_tile_x(east, zoom), zoom)
        y0 = clamp_tile(lat_to_tile_y(north, zoom), zoom)
        y1 = clamp_tile(lat_to_tile_y(south, zoom), zoom)
        if x1 < x0:
            x0, x1 = x1, x0
        if y1 < y0:
            y0, y1 = y1, y0

        count = (x1 - x0 + 1) * (y1 - y0 + 1)
        print(f"z{zoom}: x {x0}-{x1}, y {y0}-{y1}, {count} tiles")

        for x in range(x0, x1 + 1):
            for y in range(y0, y1 + 1):
                total += 1
                out = args.output / str(zoom) / str(x) / f"{y}.rgb565"
                if out.exists() and not args.force:
                    skipped += 1
                    continue

                source = find_source_tile(args.input, zoom, x, y) if args.input else None
                temp_source = None
                if source is None and args.url_template:
                    try:
                        temp_source = download_tile(args.url_template, zoom, x, y, args.user_agent, args.delay)
                        source = temp_source
                    except Exception as exc:
                        missing += 1
                        print(f"missing z{zoom}/{x}/{y}: {exc}")
                        continue

                if source is None:
                    missing += 1
                    continue

                try:
                    write_rgb565_tile(source, out)
                    written += 1
                finally:
                    if temp_source:
                        temp_source.unlink(missing_ok=True)

    print(f"done: {written} written, {skipped} skipped, {missing} missing, {total} total")
    print(f"copy this folder to SD as: /s3-lora/tiles")


if __name__ == "__main__":
    main()
