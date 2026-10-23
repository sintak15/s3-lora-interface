"""Latest MAX screw-post base and two Heltec bars on a 180 mm bed."""
import json
import math
from pathlib import Path
from layout_enclosure_plate import read_triangles,bounds,validate

ROOT=Path(__file__).resolve().parents[1]

def main():
    combined=[]
    placements=[]
    volume_before=0
    for name,x,y,count in [('enclosure-v4-base-max-screw-posts',8,8,1),
                            ('enclosure-v4-heltec-clamps',120,8,2)]:
        original=read_triangles(ROOT/(name+'.stl'))
        volume_before+=sum(b['volume_mm3'] for b in validate(original,count))
        lo,_=bounds(original)
        moved=[tuple((v[0]-lo[0]+x,v[1]-lo[1]+y,v[2]-lo[2]) for v in t) for t in original]
        validate(moved,count)
        lo,hi=bounds(moved)
        assert all(lo[i]>=7 and hi[i]<=173 for i in (0,1))
        for other in placements:
            assert any(hi[i]+5<=other['min_mm'][i] or other['max_mm'][i]+5<=lo[i] for i in (0,1))
        placements.append(dict(source=name+'.stl',min_mm=lo,max_mm=hi))
        combined.extend(moved)
    bodies=validate(combined,3)
    assert math.isclose(sum(b['volume_mm3'] for b in bodies),volume_before,abs_tol=.01)
    path=ROOT/'enclosure-v4-latest-base-and-heltec-brackets.stl'
    with path.open('w',encoding='ascii',newline='\n') as out:
        out.write('solid latest_base_and_two_heltec_brackets\n')
        for t in combined:
            out.write('facet normal 0 0 0\nouter loop\n')
            for v in t: out.write('vertex '+' '.join(repr(c) for c in v)+'\n')
            out.write('endloop\nendfacet\n')
        out.write('endsolid latest_base_and_two_heltec_brackets\n')
    validate(read_triangles(path),3)
    lo,hi=bounds(combined)
    report=dict(size_mm=[hi[i]-lo[i] for i in range(3)],bed_mm=[180,180],
                placements=placements,bodies=bodies,scaling=1,rotation_applied=False)
    path.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(path.name)
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
