#!/usr/bin/env python3
"""Drop micro shells from an ASCII STL exported by OpenSCAD.

CGAL occasionally leaves a closed sliver of a few triangles where two
surfaces are exactly tangent (finger tubes against knuckle pins). They are
far below casting resolution but make mesh checkers report extra shells.
Any connected component with fewer than MIN_VERTS vertices is removed and the
file is rewritten in place. Pure Python, no dependencies.

    python3 clean_stl.py export/*.stl
"""
import sys

MIN_VERTS = 64


def clean(path):
    facets, cur, verts = [], [], {}
    for line in open(path):
        s = line.strip()
        if s.startswith("facet normal"):
            cur = [line]
        elif s.startswith("vertex"):
            cur.append(line)
            verts.setdefault(s[7:], len(verts))
        elif s.startswith("endfacet"):
            cur.append(line)
            facets.append(cur)
        else:
            cur.append(line) if cur and not s.startswith("endsolid") else None
    parent = list(range(len(verts)))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    tri_ids = []
    for f in facets:
        ids = [verts[l.strip()[7:]] for l in f if l.strip().startswith("vertex")]
        tri_ids.append(ids)
        for b in ids[1:]:
            parent[find(b)] = find(ids[0])
    size = {}
    for v in range(len(verts)):
        r = find(v)
        size[r] = size.get(r, 0) + 1
    keep = [f for f, ids in zip(facets, tri_ids) if size[find(ids[0])] >= MIN_VERTS]
    dropped = len(facets) - len(keep)
    shells = sum(1 for s in size.values() if s >= MIN_VERTS)
    if dropped:
        with open(path, "w") as out:
            out.write("solid OpenSCAD_Model\n")
            for f in keep:
                out.writelines(f)
            out.write("endsolid OpenSCAD_Model\n")
    print(f"{path}: {len(keep)} facets, {shells} shell(s), dropped {dropped} sliver facet(s)")


if __name__ == "__main__":
    for p in sys.argv[1:]:
        clean(p)
