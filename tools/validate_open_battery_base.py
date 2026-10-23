"""Check the open holder area, base mesh and four face-entry case pilots."""
import json
import argparse
import math
from pathlib import Path
from layout_enclosure_plate import read_triangles, bounds, validate

ROOT = Path(__file__).resolve().parents[1]
ENCLOSURE_ROOT = ROOT / "hardware" / "enclosure"

def clip(poly, axis, edge, greater):
    result = []
    for a,b in zip(poly,poly[1:]+poly[:1]):
        ina = a[axis]>=edge if greater else a[axis]<=edge
        inb = b[axis]>=edge if greater else b[axis]<=edge
        if ina:
            result.append(a)
        if ina != inb:
            f = (edge-a[axis])/(b[axis]-a[axis])
            result.append(tuple(a[i]+f*(b[i]-a[i]) for i in range(3)))
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path', nargs='?', type=Path, default=ENCLOSURE_ROOT / 'enclosure-v4-base-open-battery.stl')
    path = parser.parse_args().path
    triangles = read_triangles(path)
    bodies = validate(triangles,1)
    lo,hi = bounds(triangles)
    assert all(abs(a-b)<.001 for a,b in zip([hi[i]-lo[i] for i in range(3)],[104,124,35.8]))
    # Rear print coordinates mirror X: assembly [52,100] becomes [4,52].
    # Clip all triangles to the interior of that rectangle, testing that the
    # only surface through the holder's height is the flat floor at Z=1.6.
    floor_area = 0
    for triangle in triangles:
        poly = list(triangle)
        for axis,edge,greater in [(0,4.01,True),(0,51.99,False),(1,10.01,True),(1,89.99,False)]:
            if not poly: break
            poly=clip(poly,axis,edge,greater)
        if len(poly)<3: continue
        area=abs(sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(poly,poly[1:]+poly[:1])))/2
        if area<1e-7: continue
        assert not any(1.601<v[2]<24 for v in poly), (triangle,poly)
        if all(abs(v[2]-1.6)<.001 for v in poly): floor_area+=area
    assert abs(floor_area-47.98*79.98)<.01,floor_area
    vertices=set(v for t in triangles for v in t)
    for x in (5,99):
        for y in (5,119):
            z=[v[2] for v in vertices if abs(math.hypot(v[0]-x,v[1]-y)-1.3)<.002]
            assert abs(min(z)-22)<.002 and abs(max(z)-34)<.002
    report=dict(size_mm=[hi[i]-lo[i] for i in range(3)],bodies=bodies,
                clear_battery_area_mm=[48,80],floor_thickness_mm=1.6,
                checked_holder_height_mm=20.9,tape_allowance_mm=1,
                case_pilot_diameter_mm=2.6,case_pilot_depth_mm=12,
                physical_fit_verified=False)
    path.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
