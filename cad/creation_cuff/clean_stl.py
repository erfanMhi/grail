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


def binary_to_ascii(path):
    """Rewrite a binary STL as ASCII in place so the line-based cleaner can read it."""
    import struct
    raw = open(path, "rb").read()
    if raw[:5] == b"solid" and b"facet" in raw[:2000]:
        return
    n = struct.unpack_from("<I", raw, 80)[0]
    out = ["solid OpenSCAD_Model\n"]
    off = 84
    for _ in range(n):
        f = struct.unpack_from("<12f", raw, off)
        off += 50
        out.append("  facet normal %g %g %g\n    outer loop\n" % f[0:3])
        for k in (3, 6, 9):
            out.append("      vertex %.6g %.6g %.6g\n" % f[k:k + 3])
        out.append("    endloop\n  endfacet\n")
    out.append("endsolid OpenSCAD_Model\n")
    open(path, "w").write("".join(out))


def clean(path):
    binary_to_ascii(path)
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
