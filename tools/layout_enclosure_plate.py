"""Place the default V4 enclosure on a 180 mm bed without resizing parts.

The shells stand on their lower edges. Slicer supports are REQUIRED; they are
not baked into the STL. All five loose retainers keep their flat orientation.
Run tools/export_enclosure.ps1 first to regenerate the source part meshes.
"""

import argparse
from collections import defaultdict
import json
import math
from pathlib import Path
import struct

from clean_enclosure_stl import cross, dot, edges_of, closed, subtract


# name, bed X/Y, stand on lower Y edge, expected connected bodies
LAYOUT = [
    ('front', 8, 15, True, 1),
    ('back', 8, 65, True, 1),
    ('speaker_retainer', 120, 15, False, 1),
    ('clamps', 120, 60, False, 2),
    ('antenna_retainers', 124, 90, False, 2),
]
BED = 180
MARGIN = 7


def read_triangles(path):
    data = path.read_bytes()
    if data.startswith(b'solid '):
        vertices = [tuple(float(v) for v in line.split()[1:])
                    for line in data.decode('ascii').splitlines()
                    if line.strip().startswith('vertex ')]
        return [tuple(vertices[i:i+3]) for i in range(0, len(vertices), 3)]
    count = struct.unpack_from('<I', data, 80)[0]
    if len(data) != 84 + count * 50:
        raise ValueError(f'{path}: expected binary STL')
    return [tuple(tuple(record[j:j+3]) for j in (3, 6, 9))
            for record in struct.iter_unpack('<12fH', data[84:])]


def bounds(triangles):
    points = [v for triangle in triangles for v in triangle]
    return ([min(p[i] for p in points) for i in range(3)],
            [max(p[i] for p in points) for i in range(3)])


def validate(triangles, expected_bodies):
    vertices, lookup, faces = [], {}, []
    for triangle in triangles:
        face = []
        for vertex in triangle:
            if vertex not in lookup:
                lookup[vertex] = len(vertices)
                vertices.append(vertex)
            face.append(lookup[vertex])
        faces.append(tuple(face))
        n = cross(subtract(triangle[1], triangle[0]), subtract(triangle[2], triangle[0]))
        if dot(n, n) == 0:
            raise ValueError('Degenerate triangle')
    edges = edges_of(faces)
    if not closed(edges):
        raise ValueError('Open edges or inconsistent winding')
    adjacent = defaultdict(set)
    for entries in edges.values():
        a, b = entries[0][0], entries[1][0]
        adjacent[a].add(b)
        adjacent[b].add(a)
    unseen = set(range(len(faces)))
    bodies = []
    while unseen:
        pending = [unseen.pop()]
        component = []
        while pending:
            face = pending.pop()
            component.append(face)
            for other in adjacent[face]:
                if other in unseen:
                    unseen.remove(other)
                    pending.append(other)
        volume = sum(dot(triangles[i][0], cross(triangles[i][1], triangles[i][2])) / 6
                     for i in component)
        contact_area = sum(math.sqrt(dot(n, n)) / 2
                           for i in component if all(abs(v[2]) < 1e-6 for v in triangles[i])
                           for n in [cross(subtract(triangles[i][1], triangles[i][0]),
                                           subtract(triangles[i][2], triangles[i][0]))])
        if volume <= 0 or contact_area <= 0:
            raise ValueError('Each body must have positive volume and a face on the bed')
        bodies.append({'volume_mm3': round(volume, 3), 'bed_contact_mm2': round(contact_area, 3)})
    if len(bodies) != expected_bodies:
        raise ValueError(f'Expected {expected_bodies} bodies, found {len(bodies)}')
    return bodies


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--parts', type=Path, default=root / 'build/enclosure-v4')
    parser.add_argument('--output', type=Path,
                        default=root / 'build/enclosure-v4/enclosure-v4-all-parts-180mm.stl')
    args = parser.parse_args()
    combined, placements = [], []
    for name, x, y, upright, body_count in LAYOUT:
        triangles = read_triangles(args.parts / (name + '.stl'))
        original_volume = sum(dot(t[0], cross(t[1], t[2])) / 6 for t in triangles)
        if upright:
            # Exact 90 degree rotation about X: (x,y,z) -> (x,-z,y).
            triangles = [tuple((v[0], -v[2], v[1]) for v in t) for t in triangles]
        lo, _ = bounds(triangles)
        triangles = [tuple((v[0]-lo[0]+x, v[1]-lo[1]+y, v[2]-lo[2]) for v in t)
                     for t in triangles]
        bodies = validate(triangles, body_count)
        if not math.isclose(sum(b['volume_mm3'] for b in bodies), original_volume, abs_tol=0.01):
            raise ValueError(f'{name}: material volume changed')
        lo, hi = bounds(triangles)
        if any(lo[i] < MARGIN or hi[i] > BED-MARGIN for i in (0, 1)):
            raise ValueError(f'{name}: outside printable bed margin')
        for other in placements:
            if all(min(hi[i], other['max_mm'][i]) > max(lo[i], other['min_mm'][i]) for i in (0, 1)):
                raise ValueError(f'{name}: overlaps {other["part"]}')
        placements.append({'part': name, 'min_mm': lo, 'max_mm': hi,
                           'orientation': 'on lower edge' if upright else 'flat', 'bodies': bodies})
        combined.extend(triangles)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # ASCII preserves coordinates through rigid transforms. Requantizing to
    # float32 binary here can collapse distinct vertices in thin CGAL facets.
    with args.output.open('w', encoding='ascii', newline='\n') as stream:
        stream.write('solid V4_180mm_KEEP_ORIENTATION_SUPPORTS_REQUIRED\n')
        for triangle in combined:
            n = cross(subtract(triangle[1], triangle[0]), subtract(triangle[2], triangle[0]))
            length = math.sqrt(dot(n, n))
            stream.write(' facet normal ' + ' '.join(format(v/length, '.17g') for v in n) + '\n  outer loop\n')
            for point in triangle:
                stream.write('   vertex ' + ' '.join(format(v, '.17g') for v in point) + '\n')
            stream.write('  endloop\n endfacet\n')
        stream.write('endsolid V4_180mm_KEEP_ORIENTATION_SUPPORTS_REQUIRED\n')
    # Validate the actual exported coordinates as well as in-memory transforms.
    exported = read_triangles(args.output)
    bodies = validate(exported, 7)
    lo, hi = bounds(exported)
    if any(lo[i] < MARGIN-1e-4 or hi[i] > BED-MARGIN+1e-4 for i in (0, 1)) or abs(lo[2]) > 1e-6:
        raise ValueError('Export outside bed limits')
    report = {'bed_mm': [BED, BED], 'scale': 1, 'supports_required': True,
              'included': '2 shells, 2 Heltec clamps, 2 GPS rails, 1 speaker frame; no optional fit coupon',
              'min_mm': lo, 'max_mm': hi, 'size_mm': [round(b-a, 3) for a, b in zip(lo, hi)],
              'body_count': len(bodies), 'placements': placements}
    args.output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{args.output}: {report["size_mm"]} mm, 7 closed bodies, all touching bed; supports required')


if __name__ == '__main__':
    main()
