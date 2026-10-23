"""Verify the new USB opening and the filled old opening on the rear STL."""
import json
import argparse
from pathlib import Path
from layout_enclosure_plate import read_triangles
from validate_open_battery_base import clip

ROOT = Path(__file__).resolve().parents[1]
ENCLOSURE_ROOT = ROOT / "hardware" / "enclosure"

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path',nargs='?',type=Path,default=ENCLOSURE_ROOT / 'enclosure-v4-base-power-usb-bottom.stl')
    path=parser.parse_args().path
    triangles=read_triangles(path)
    def outer_wall_area(y0,y1):
        total=0
        for t in triangles:
            if not all(abs(v[0]-104)<.001 for v in t): continue
            poly=list(t)
            for axis,edge,greater in [(1,y0,True),(1,y1,False),(2,3.5,True),(2,15.5,False)]:
                if not poly: break
                poly=clip(poly,axis,edge,greater)
            if len(poly)>=3:
                total+=abs(sum(a[1]*b[2]-b[1]*a[2] for a,b in zip(poly,poly[1:]+poly[:1])))/2
        return total
    new=outer_wall_area(18,36)
    old=outer_wall_area(56,74)
    assert abs(new)<.001,new
    assert abs(old-216)<.001,old
    meta=path.with_suffix('.json')
    report=json.loads(meta.read_text())
    report.update(power_usb_center_from_bottom_wall_mm=27,
                  power_usb_center_from_rear_face_mm=9.5,
                  new_opening_wall_area_mm2=new,old_opening_filled_area_mm2=old)
    meta.write_text(json.dumps(report,indent=2)+'\n')
    print('New 18 x 12 mm USB opening is clear; old opening is filled.')

if __name__=='__main__': main()
