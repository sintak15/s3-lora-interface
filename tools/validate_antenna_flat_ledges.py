"""Check full 7.5 mm antenna stack between ledges and bracket posts."""
import json
import math
from pathlib import Path
from layout_enclosure_plate import read_triangles,bounds,validate

ROOT=Path(__file__).resolve().parents[1]

def main():
    path=ROOT/'enclosure-v4-front-antenna-ledges-lower.stl'
    triangles=read_triangles(path)
    bodies=validate(triangles,1)
    vs=set(v for t in triangles for v in t)
    for x in (67.5,70.2,93.8,96.5):
        for y in (97,100,107,110):
            assert any(abs(v[0]-x)<.002 and abs(v[1]-y)<.002 and abs(v[2]-3.8)<.002 for v in vs),(x,y)
    def circle(x,y,r,z):
        return sum(abs(v[2]-z)<.002 and abs(math.hypot(v[0]-x,v[1]-y)-r)<.002 for v in vs)
    assert all(circle(x,y,2.1,11.3)>40 for x in (65,99) for y in (99,108))
    assert all(circle(x,y,.8,18)>40 for x in (60.6,92.98) for y in (92.6,114.33))
    assert all(circle(x,y,1.25,20.1)>40 for x in (60,94.2) for y in (20,58))
    coupon=validate(read_triangles(ROOT/'enclosure-v4-antenna-ledges-coupon.stl'),1)
    assert abs(3.8+7.5-11.3)<1e-8
    lo,hi=bounds(triangles)
    report=dict(size_mm=[hi[i]-lo[i] for i in range(3)],bodies=bodies,
                ledge_z_mm=3.8,round_bracket_post_z_mm=11.3,
                antenna_total_thickness_mm=7.5,stack_top_z_mm=11.3,
                ledge_lowering_mm=7.5,neo6m_seat_z_mm=18,
                heltec_post_z_mm=20.1,coupon=coupon,physical_fit_verified=False)
    path.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
