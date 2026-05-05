#!/usr/bin/env python3
"""Render simple offline street maps from an OpenStreetMap .osm.pbf file.

This writes the CYD firmware's SD-card tile layout directly:
  <sd-root>/s3-lora/tiles/<z>/<x>/<y>.rgb565

The output is intentionally simple and readable on a small 240x320 screen:
water/land detail is omitted, while streets, highways, and rail lines are drawn.
"""

from __future__ import annotations

import argparse
import math
import sys
from collections import defaultdict
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Missing dependency: Pillow. Install with: python -m pip install pillow", file=sys.stderr)
    raise SystemExit(2)


TILE_SIZE = 256

# Full Minnesota plus roughly the western half of Wisconsin.
MN_WEST_WI_BOUNDS = (42.45, -97.50, 49.50, -89.00)  # south, west, north, east

ROAD_STYLES = {
    "motorway": (0, 6, (222, 96, 69)),
    "trunk": (1, 5, (232, 126, 72)),
    "primary": (2, 4, (236, 164, 84)),
    "secondary": (3, 3, (230, 196, 106)),
    "tertiary": (4, 2, (214, 206, 154)),
    "unclassified": (5, 1, (176, 181, 172)),
    "residential": (6, 1, (196, 201, 194)),
    "service": (7, 1, (158, 164, 158)),
    "living_street": (6, 1, (196, 201, 194)),
    "road": (6, 1, (176, 181, 172)),
    "track": (8, 1, (147, 137, 105)),
    "path": (9, 1, (121, 143, 116)),
    "cycleway": (9, 1, (107, 151, 120)),
    "footway": (9, 1, (137, 151, 120)),
}


def lon_to_tile_x(lon: float, zoom: int) -> int:
    return int(math.floor((lon + 180.0) / 360.0 * (1 << zoom)))


def lat_to_tile_y(lat: float, zoom: int) -> int:
    lat = max(min(lat, 85.05112878), -85.05112878)
    lat_rad = math.radians(lat)
    return int(math.floor((1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * (1 << zoom)))


def lon_to_global_pixel_x(lon: float, zoom: int) -> float:
    return (lon + 180.0) / 360.0 * (1 << zoom) * TILE_SIZE


def lat_to_global_pixel_y(lat: float, zoom: int) -> float:
    lat = max(min(lat, 85.05112878), -85.05112878)
    lat_rad = math.radians(lat)
    return (1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * (1 << zoom) * TILE_SIZE


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


def output_root(out: Path) -> Path:
    if out.drive and not out.root:
        return Path(out.drive + "\\")
    return out


def tile_path(root: Path, z: int, x: int, y: int) -> Path:
    return root / "s3-lora" / "tiles" / str(z) / str(x) / f"{y}.rgb565"


def way_intersects_bounds(coords: list[tuple[float, float]], bounds: tuple[float, float, float, float]) -> bool:
    south, west, north, east = bounds
    for lat, lon in coords:
        if south <= lat <= north and west <= lon <= east:
            return True
    lats = [lat for lat, _ in coords]
    lons = [lon for _, lon in coords]
    return max(lats) >= south and min(lats) <= north and max(lons) >= west and min(lons) <= east


def line_tile_range(coords: list[tuple[float, float]], zoom: int) -> tuple[int, int, int, int]:
    xs = [lon_to_tile_x(lon, zoom) for _, lon in coords]
    ys = [lat_to_tile_y(lat, zoom) for lat, _ in coords]
    return min(xs), max(xs), min(ys), max(ys)


def write_rgb565(image: Image.Image, output: Path) -> None:
    image = image.convert("RGB")
    pixels = image.tobytes()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as handle:
        for i in range(0, len(pixels), 3):
            r, g, b = pixels[i], pixels[i + 1], pixels[i + 2]
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            handle.write(bytes((rgb565 & 0xFF, rgb565 >> 8)))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Render OSM PBF street tiles for the CYD SD card.")
    parser.add_argument("--pbf", type=Path, nargs="+", required=True, help="one or more OpenStreetMap .osm.pbf files")
    parser.add_argument("--out", type=Path, required=True, help="SD card root, for example E:\\")
    parser.add_argument("--zoom", default="10-14", help="zoom level, comma list, or range; default 10-14")
    parser.add_argument(
        "--bounds",
        nargs=4,
        type=float,
        metavar=("SOUTH", "WEST", "NORTH", "EAST"),
        default=MN_WEST_WI_BOUNDS,
        help="render bounds; default is Minnesota plus western Wisconsin",
    )
    parser.add_argument("--dry-run", action="store_true", help="count tiles and roads without writing")
    parser.add_argument("--overwrite", action="store_true", help="replace existing .rgb565 files")
    parser.add_argument("--roads-only", action="store_true", help="skip rail lines")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    for pbf in args.pbf:
        if not pbf.exists():
            print(f"PBF file not found: {pbf}", file=sys.stderr)
            return 2

    try:
        import osmium
    except ImportError:
        print("Missing dependency: osmium. Install with: python -m pip install osmium", file=sys.stderr)
        return 2

    bounds = tuple(args.bounds)
    zooms = parse_zoom_spec(args.zoom)

    class WayCollector(osmium.SimpleHandler):
        def __init__(self) -> None:
            super().__init__()
            self.ways: list[dict] = []
            self.node_refs: set[int] = set()

        def way(self, way) -> None:
            highway = way.tags.get("highway")
            railway = way.tags.get("railway")
            if highway in ROAD_STYLES:
                rank, width, color = ROAD_STYLES[highway]
            elif railway and not args.roads_only:
                rank, width, color = (10, 1, (100, 104, 112))
            else:
                return
            refs = [node.ref for node in way.nodes]
            if len(refs) < 2:
                return
            self.ways.append({"refs": refs, "rank": rank, "width": width, "color": color})
            self.node_refs.update(refs)

    class NodeCollector(osmium.SimpleHandler):
        def __init__(self, wanted: set[int]) -> None:
            super().__init__()
            self.wanted = wanted
            self.nodes: dict[int, tuple[float, float]] = {}

        def node(self, node) -> None:
            if node.id in self.wanted and node.location.valid():
                self.nodes[node.id] = (node.location.lat, node.location.lon)

    all_way_records: list[dict] = []
    all_node_refs: set[int] = set()
    for pbf in args.pbf:
        print(f"Reading ways from {pbf}...")
        way_collector = WayCollector()
        way_collector.apply_file(str(pbf), locations=False)
        print(f"  candidate ways: {len(way_collector.ways):,}; referenced nodes: {len(way_collector.node_refs):,}")
        all_way_records.extend(way_collector.ways)
        all_node_refs.update(way_collector.node_refs)

    print("Reading referenced nodes...")
    node_collector = NodeCollector(all_node_refs)
    for pbf in args.pbf:
        print(f"  nodes from {pbf}...")
        node_collector.apply_file(str(pbf), locations=False)
    print(f"Loaded nodes: {len(node_collector.nodes):,}")

    ways: list[dict] = []
    for way in all_way_records:
        coords = [node_collector.nodes[ref] for ref in way["refs"] if ref in node_collector.nodes]
        if len(coords) < 2 or not way_intersects_bounds(coords, bounds):
            continue
        ways.append({**way, "coords": coords})
    print(f"Ways inside bounds: {len(ways):,}")

    south, west, north, east = bounds
    tile_ranges = {}
    total_tiles = 0
    for zoom in zooms:
        min_x = lon_to_tile_x(west, zoom)
        max_x = lon_to_tile_x(east, zoom)
        min_y = lat_to_tile_y(north, zoom)
        max_y = lat_to_tile_y(south, zoom)
        tile_ranges[zoom] = (min_x, max_x, min_y, max_y)
        count = (max_x - min_x + 1) * (max_y - min_y + 1)
        total_tiles += count
        print(f"z{zoom}: x={min_x}..{max_x}, y={min_y}..{max_y}, tiles={count:,}")

    approx_gib = total_tiles * TILE_SIZE * TILE_SIZE * 2 / (1024 * 1024 * 1024)
    print(f"Total tiles: {total_tiles:,}; raw RGB565 size: {approx_gib:.2f} GiB")
    if args.dry_run:
        print("Dry run only; no files written.")
        return 0

    print("Building per-tile road index...")
    indexes: dict[int, dict[tuple[int, int], list[int]]] = {}
    for zoom in zooms:
        min_x, max_x, min_y, max_y = tile_ranges[zoom]
        index: dict[tuple[int, int], list[int]] = defaultdict(list)
        for way_index, way in enumerate(ways):
            way_min_x, way_max_x, way_min_y, way_max_y = line_tile_range(way["coords"], zoom)
            for x in range(max(min_x, way_min_x), min(max_x, way_max_x) + 1):
                for y in range(max(min_y, way_min_y), min(max_y, way_max_y) + 1):
                    index[(x, y)].append(way_index)
        indexes[zoom] = index

    root = output_root(args.out)
    written = 0
    skipped = 0
    bg = (235, 238, 230)
    for zoom in zooms:
        min_x, max_x, min_y, max_y = tile_ranges[zoom]
        index = indexes[zoom]
        for x in range(min_x, max_x + 1):
            for y in range(min_y, max_y + 1):
                out = tile_path(root, zoom, x, y)
                if out.exists() and not args.overwrite:
                    skipped += 1
                    continue
                image = Image.new("RGB", (TILE_SIZE, TILE_SIZE), bg)
                draw = ImageDraw.Draw(image)
                tile_origin_x = x * TILE_SIZE
                tile_origin_y = y * TILE_SIZE
                for way_index in sorted(index.get((x, y), []), key=lambda i: ways[i]["rank"], reverse=True):
                    way = ways[way_index]
                    points = [
                        (
                            int(round(lon_to_global_pixel_x(lon, zoom) - tile_origin_x)),
                            int(round(lat_to_global_pixel_y(lat, zoom) - tile_origin_y)),
                        )
                        for lat, lon in way["coords"]
                    ]
                    draw.line(points, fill=way["color"], width=max(1, way["width"]), joint="curve")
                write_rgb565(image, out)
                written += 1
                if written % 500 == 0:
                    print(f"wrote {written:,}/{total_tiles:,} tiles...")

    print(f"Done. wrote={written:,} skipped={skipped:,}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
