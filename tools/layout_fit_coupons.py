"""Combine seven fit coupons on a 180 mm bed without scaling or rotation."""
import json
import math
import argparse
from pathlib import Path

from layout_enclosure_plate import read_triangles, bounds, validate

ROOT = Path(__file__).resolve().parents[1]
PARTS = ROOT / 'hardware' / 'enclosure' / 'fit-coupons'
LAYOUT = [
    ('cyd_mounts', 8, 8),
    ('cyd_usb', 64, 8),
    ('heltec', 64, 38),
    ('power', 64, 52),
    ('battery', 106, 52),
    ('max', 64, 72),
    ('heltec_clamp', 64, 94),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--labeled', action='store_true')
    parser.add_argument('--lightweight', action='store_true')
    parser.add_argument('--screen', action='store_true')
    args = parser.parse_args()
    base = PARTS / 'lightweight' if args.lightweight else PARTS
    parts = base / 'labeled' if args.labeled else base
    layout = [(n,x,y) for n,x,y in LAYOUT]
    if args.labeled:
        layout = [('cyd_mounts',8,8),('cyd_usb',64,8),('heltec',64,46),
                  ('power',64,70),('battery',106,70),('max',64,100),
                  ('heltec_clamp',64,130)]
    if args.screen:
        layout = [('cyd_mounts',8,8),('cyd_usb',64,8),('heltec_screen',64,46),
                  ('power',112,46),('battery',112,76),('max',112,106),
                  ('heltec_clamp',64,120),('heltec_clamp',64,143)]
    combined, placements = [], []
    for name, x, y in layout:
        original = read_triangles(parts / f'{name}.stl')
        before = validate(original, 1)
        lo, _ = bounds(original)
        moved = [tuple((v[0]-lo[0]+x, v[1]-lo[1]+y, v[2]-lo[2])
                       for v in t) for t in original]
        after = validate(moved, 1)
        assert math.isclose(before[0]['volume_mm3'], after[0]['volume_mm3'], abs_tol=0.002)
        lo, hi = bounds(moved)
        assert all(lo[i] >= 7 and hi[i] <= 173 for i in (0, 1))
        for other in placements:
            assert any(hi[i]+3 <= other['min_mm'][i] or
                       other['max_mm'][i]+3 <= lo[i] for i in (0, 1)), name
        placements.append(dict(part=name, min_mm=lo, max_mm=hi))
        combined.extend(moved)
    bodies = validate(combined, len(layout))
    target = base / ('all-coupons-labeled-180mm.stl' if args.labeled else 'all-coupons-180mm.stl')
    if args.screen:
        target = base / 'all-coupons-screen-fit-180mm.stl'
    with target.open('w', encoding='ascii', newline='\n') as stream:
        stream.write('solid V4_FIT_COUPONS_100_PERCENT\n')
        for triangle in combined:
            stream.write('facet normal 0 0 0\nouter loop\n')
            for v in triangle:
                stream.write('vertex ' + ' '.join(repr(c) for c in v) + '\n')
            stream.write('endloop\nendfacet\n')
        stream.write('endsolid V4_FIT_COUPONS_100_PERCENT\n')
    validate(read_triangles(target), len(layout))
    lo, hi = bounds(combined)
    target.with_suffix('.json').write_text(json.dumps(dict(
        bed_mm=[180,180], size_mm=[hi[i]-lo[i] for i in range(3)],
        placements=placements, bodies=bodies), indent=2)+'\n')
    print(f'{target}: {len(layout)} closed bodies, bed contact, unchanged volumes, >=3 mm gaps')
    print('Layout size (mm):', [round(hi[i]-lo[i], 2) for i in range(3)])


if __name__ == '__main__':
    main()
