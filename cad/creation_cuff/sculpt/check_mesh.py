"""Mesh sanity for casting: shells, open edges, non-manifold edges, volume.
    python check_mesh.py mesh.ply"""
import struct
import sys

import numpy as np


def read_ply(path):
    with open(path, "rb") as f:
        nv = nf = 0
        props = 0
        while True:
            line = f.readline().decode().strip()
            if line.startswith("element vertex"):
                nv = int(line.split()[-1])
            elif line.startswith("element face"):
                nf = int(line.split()[-1])
            elif line.startswith("property float"):
                props += 1
            elif line == "end_header":
                break
        v = np.frombuffer(f.read(nv * props * 4), dtype="<f4").reshape(nv, props)[:, :3]
        raw = np.frombuffer(f.read(nf * 13), dtype=np.uint8).reshape(nf, 13)
        faces = raw[:, 1:].copy().view("<u4").reshape(nf, 3)
    return v.astype(np.float64), faces.astype(np.int64)


def main():
    v, f = read_ply(sys.argv[1])
    # edges
    e = np.sort(np.concatenate([f[:, [0, 1]], f[:, [1, 2]], f[:, [2, 0]]]), axis=1)
    key = e[:, 0] * (len(v) + 1) + e[:, 1]
    _, counts = np.unique(key, return_counts=True)
    open_edges = int((counts == 1).sum())
    nonman = int((counts > 2).sum())
    # shells by union-find over faces' vertices
    parent = np.arange(len(v))

    def find(x):
        root = x
        while parent[root] != root:
            root = parent[root]
        while parent[x] != root:
            parent[x], x = root, parent[x]
        return root
    for a, b, c in f:
        ra, rb, rc = find(a), find(b), find(c)
        if ra != rb:
            parent[rb] = ra
        rc = find(c)
        ra = find(a)
        if ra != rc:
            parent[rc] = ra
    roots = np.array([find(i) for i in np.unique(f)])
    shells, sizes = np.unique(roots, return_counts=True)
    order = np.argsort(-sizes)
    # signed volume
    a, b, c = v[f[:, 0]], v[f[:, 1]], v[f[:, 2]]
    vol = float(np.einsum("ij,ij->i", a, np.cross(b, c)).sum() / 6.0)
    lo, hi = v.min(0), v.max(0)
    print(f"vertices {len(v)}  triangles {len(f)}")
    print(f"shells {len(shells)}  largest {sizes[order[:5]].tolist()}")
    print(f"open edges {open_edges}  non-manifold edges {nonman}")
    print(f"volume {vol:.1f} mm^3  ->  sterling {vol * 10.36e-3:.2f} g, 18k gold {vol * 15.6e-3:.2f} g")
    print(f"bounds {np.round(lo, 2).tolist()} .. {np.round(hi, 2).tolist()}")
    if len(shells) > 1:
        used = np.unique(f)
        for s in shells[order[1:8]]:
            pts = v[used[roots == s]]
            print(f"  shell of {len(pts)} verts at {np.round(pts.mean(0), 2).tolist()} size {np.round(pts.max(0) - pts.min(0), 2).tolist()}")


main()
