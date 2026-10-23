"""Attach removable printed name tags without changing fit surfaces."""
from pathlib import Path
import subprocess
import os
import argparse
from layout_enclosure_plate import read_triangles, bounds, validate

ROOT = Path(__file__).resolve().parents[1]
PARTS = ROOT / 'fit-coupons'
OUT = PARTS / 'labeled'
LABELS = {'cyd_mounts':'CYD HOLES', 'cyd_usb':'CYD USB', 'heltec':'HELTEC',
          'heltec_clamp':'CLAMP', 'power':'POWER', 'battery':'BATTERY', 'max':'MAX'}

def main():
    global PARTS, OUT
    parser = argparse.ArgumentParser()
    parser.add_argument('--lightweight', action='store_true')
    parser.add_argument('--screen', action='store_true')
    args = parser.parse_args()
    if args.lightweight:
        PARTS = PARTS / 'lightweight'
        OUT = PARTS / 'labeled'
    OUT.mkdir(exist_ok=True)
    cad = Path(os.environ['ProgramFiles']) / 'OpenSCAD' / 'openscad.com'
    labels = {'heltec_screen':'OLED FIT'} if args.screen else LABELS
    for name, label in labels.items():
        _, hi = bounds(read_triangles(PARTS / f'{name}.stl'))
        width = hi[0]
        # 0.6 mm attachment tab reaches the base without covering fit surfaces.
        # Tag sits outside the part footprint; snap off before fitting boards.
        scad = OUT / f'{name}.scad'
        scad.write_text(f'''union() {{
 import("../{name}.stl", convexity=10);
 translate([0,-9,0]) cube([{width},7,{0.6 if args.lightweight else 0.8}]);
 translate([{5 if name == 'heltec_screen' else 1},-2.1,0]) cube([3,5.1,0.6]);
 translate([{width/2},-5.5,{0.5 if args.lightweight else 0.7}]) linear_extrude({0.5 if args.lightweight else 0.7})
 text("{label}",size=3,font="Liberation Sans",halign="center",valign="center");
}}
''')
        target = OUT / f'{name}.stl'
        result = subprocess.run([str(cad), '-o', str(target), '--export-format', 'binstl', str(scad)], check=True, capture_output=True, text=True)
        if 'ERROR:' in result.stderr or 'WARNING:' in result.stderr:
            raise RuntimeError(result.stderr)
        subprocess.run(['python', str(ROOT/'tools/clean_enclosure_stl.py'), str(target)], check=True)
        validate(read_triangles(target), 1)
    print('All requested tags are connected and have bed contact.')

if __name__ == '__main__':
    main()
