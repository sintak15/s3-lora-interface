"""Verify the measured CYD seats, reinforced posts and widened GPS supports."""
import json
import math
from pathlib import Path
from layout_enclosure_plate import read_triangles,bounds,validate

ROOT=Path(__file__).resolve().parents[1]
ENCLOSURE_ROOT = ROOT / "hardware" / "enclosure"

def main():
    path=ENCLOSURE_ROOT / 'enclosure-v4-front-heavy-ties.stl'
    triangles=read_triangles(path)
    bodies=validate(triangles,1)
    vertices=set(v for t in triangles for v in t)
    def circle(x,y,r,z):
        return sum(abs(v[2]-z)<.002 and abs(math.hypot(v[0]-x,v[1]-y)-r)<.002 for v in vertices)
    for x in (11,53):
        for y in (15,93):
            assert circle(x,y,3,5.7)>20
    for x in (60,94.2):
        for y in (20,58):
            assert circle(x,y,3.6,20.1)>20
            assert circle(x,y,1.25,20.1)>40
    for x in (64,95):
        for y in (99,108):
            assert circle(x,y,2.1,12.9)>40
            assert circle(x,y,.8,12.9)>40
    for x in (5,99):
        for y in (5,119):
            assert circle(x,y,1.7,0)>40
            assert circle(x,y,1.7,16)>40
    rails=validate(read_triangles(ENCLOSURE_ROOT / 'enclosure-v4-gps-rails-clearance.stl'),2)
    lo,hi=bounds(triangles)
    report=dict(size_mm=[hi[i]-lo[i] for i in range(3)],bodies=bodies,
                cyd_post_height_above_floor_mm=3.3,heltec_post_diameter_mm=7.2,
                heltec_root_diameter_before_flat_mm=9,gps_extra_clearance_per_side_mm=.5,
                case_clearance_diameter_mm=3.4,gps_rail_bodies=rails,
                physical_fit_verified=False)
    path.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
