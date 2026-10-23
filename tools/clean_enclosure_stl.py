"""Retriangulate collinear binary STL facets without moving vertices.

OpenSCAD's float32 STL export can flatten very thin CGAL triangles. Replace
each collinear triangle and its neighbour across the long edge with the two
proper triangles obtained by splitting that neighbour at the middle vertex.
This preserves the surface, volume and closed edge topology. No mesh repair
that moves vertices or fills holes is performed.
"""

import argparse
from collections import defaultdict
import math
from pathlib import Path
import struct


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def normal(face, vertices):
    a, b, c = (vertices[i] for i in face)
    return cross(subtract(b, a), subtract(c, a))


def edges_of(faces):
    edges = defaultdict(list)
    for index, face in enumerate(faces):
        for a, b in zip(face, face[1:]+face[:1]):
            edges[tuple(sorted((a, b)))].append((index, a, b))
    return edges


def closed(edges):
    return all(len(e) == 2 and e[0][1:] == e[1][1:][::-1] for e in edges.values())


def measures(faces, vertices):
    area, volume = 0.0, 0.0
    for face in faces:
        a, b, c = (vertices[i] for i in face)
        area += math.sqrt(dot(normal(face, vertices), normal(face, vertices)))/2
        volume += dot(a, cross(b, c))/6
    return area, volume


def clean(path):
    data = path.read_bytes()
    if len(data) < 84:
        raise ValueError('Expected a binary STL')
    count = struct.unpack_from('<I', data, 80)[0]
    if len(data) != 84 + count*50:
        raise ValueError('Expected a binary STL')
    vertices, lookup, faces = [], {}, []
    for index in range(count):
        record = struct.unpack_from('<12fH', data, 84 + index*50)
        face = []
        for j in (3, 6, 9):
            vertex = tuple(record[j:j+3])
            if vertex not in lookup:
                lookup[vertex] = len(vertices)
                vertices.append(vertex)
            face.append(lookup[vertex])
        faces.append(tuple(face))
    if not closed(edges_of(faces)):
        raise ValueError('Mesh edges are open or inconsistently wound; refusing to modify')
    before = measures(faces, vertices)
    repaired = 0
    while True:
        flat = [i for i, f in enumerate(faces) if dot(normal(f, vertices), normal(f, vertices)) == 0]
        if not flat:
            break
        edges = edges_of(faces)
        changed = False
        for index in flat:
            face = faces[index]
            if len(set(face)) != 3:
                continue
            a, c = max(((face[j], face[(j+1) % 3]) for j in range(3)),
                       key=lambda e: dot(subtract(vertices[e[0]], vertices[e[1]]), subtract(vertices[e[0]], vertices[e[1]])))
            b = next(v for v in face if v not in (a, c))
            ac = subtract(vertices[c], vertices[a])
            fraction = dot(subtract(vertices[b], vertices[a]), ac)/dot(ac, ac)
            if not 0 < fraction < 1:
                continue
            neighbour, first, last = next(e for e in edges[tuple(sorted((a, c)))] if e[0] != index)
            if dot(normal(faces[neighbour], vertices), normal(faces[neighbour], vertices)) == 0:
                continue
            other = next(v for v in faces[neighbour] if v not in (first, last))
            faces[index] = (first, b, other)
            faces[neighbour] = (b, last, other)
            repaired += 1
            changed = True
            break
        if not changed:
            raise ValueError('Unsupported degenerate facets; refusing to modify')
    after = measures(faces, vertices)
    if not closed(edges_of(faces)) or not all(math.isclose(a, b, rel_tol=1e-10, abs_tol=1e-7) for a, b in zip(before, after)):
        raise ValueError('Surface/volume/topology changed; refusing to write')
    if repaired:
        output = bytearray(data[:84])
        for face in faces:
            n = normal(face, vertices)
            length = math.sqrt(dot(n, n))
            output.extend(struct.pack('<12fH', *(v/length for v in n),
                                      *(value for index in face for value in vertices[index]), 0))
        temporary = path.with_suffix('.cleaning.stl')
        temporary.write_bytes(output)
        temporary.replace(path)
    print(f'{path.name}: closed edges; {repaired} collinear facets retriangulated; vertices, area and volume preserved')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stl', type=Path)
    clean(parser.parse_args().stl)
