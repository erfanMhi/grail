"""Turn the mesher's raw surface into a casting-ready mesh.

    python finish_mesh.py raw.ply out_basename [--target 1200000]

Steps: keep the largest shell (drops sub-voxel specks), repair the few
non-manifold edges surface nets leaves where two sheets share a voxel
(MeshFix), decimate with quadric error metrics to the target size, then
write a binary STL, a 3MF and a PLY with smooth normals for rendering.
"""
import argparse
import struct
import time
import zipfile

import fast_simplification
import numpy as np
import pymeshfix


def read_ply(path):
    with open(path, "rb") as f:
        nv = nf = props = 0
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


def largest_shell(v, f):
    n = len(v)
    parent = np.arange(n)

    def find(x):
        r = x
        while parent[r] != r:
            r = parent[r]
        while parent[x] != r:
            parent[x], x = r, parent[x]
        return r
    for a, b, c in f:
        ra, rb, rc = find(a), find(b), find(c)
        if ra != rb:
            parent[rb] = ra
        if find(c) != ra:
            parent[find(c)] = ra
    roots = np.array([find(i) for i in range(n)])
    face_root = roots[f[:, 0]]
    vals, counts = np.unique(face_root, return_counts=True)
    keep = face_root == vals[np.argmax(counts)]
    f = f[keep]
    used = np.unique(f)
    remap = -np.ones(n, dtype=np.int64)
    remap[used] = np.arange(len(used))
    return v[used], remap[f], int(len(vals) - 1)


def stats(v, f):
    e = np.sort(np.concatenate([f[:, [0, 1]], f[:, [1, 2]], f[:, [2, 0]]]), axis=1)
    key = e[:, 0] * (len(v) + 1) + e[:, 1]
    _, counts = np.unique(key, return_counts=True)
    a, b, c = v[f[:, 0]], v[f[:, 1]], v[f[:, 2]]
    vol = float(np.einsum("ij,ij->i", a, np.cross(b, c)).sum() / 6.0)
    return int((counts == 1).sum()), int((counts > 2).sum()), vol


def vertex_normals(v, f):
    fn = np.cross(v[f[:, 1]] - v[f[:, 0]], v[f[:, 2]] - v[f[:, 0]])
    vn = np.zeros_like(v)
    for k in range(3):
        np.add.at(vn, f[:, k], fn)
    vn /= np.maximum(np.linalg.norm(vn, axis=1, keepdims=True), 1e-20)
    return vn


def write_stl(path, v, f, title):
    tri = v[f].astype(np.float32)
    n = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-20)
    rec = np.zeros(len(f), dtype=[("n", "<f4", 3), ("v", "<f4", (3, 3)), ("a", "<u2")])
    rec["n"], rec["v"] = n, tri
    with open(path, "wb") as fh:
        fh.write(title.encode()[:80].ljust(80, b" "))
        fh.write(struct.pack("<I", len(f)))
        fh.write(rec.tobytes())


def write_ply(path, v, f):
    vn = vertex_normals(v, f)
    with open(path, "wb") as fh:
        fh.write((f"ply\nformat binary_little_endian 1.0\nelement vertex {len(v)}\n"
                  "property float x\nproperty float y\nproperty float z\nproperty float nx\nproperty float ny\nproperty float nz\n"
                  f"element face {len(f)}\nproperty list uchar uint vertex_indices\nend_header\n").encode())
        fh.write(np.hstack([v, vn]).astype("<f4").tobytes())
        rec = np.zeros(len(f), dtype=[("n", "u1"), ("i", "<u4", 3)])
        rec["n"], rec["i"] = 3, f
        fh.write(rec.tobytes())


def write_3mf(path, v, f, name):
    verts = "\n".join(f'<vertex x="{x:.5f}" y="{y:.5f}" z="{z:.5f}"/>' for x, y, z in v)
    tris = "\n".join(f'<triangle v1="{a}" v2="{b}" v3="{c}"/>' for a, b, c in f)
    model = ('<?xml version="1.0" encoding="UTF-8"?>\n'
             '<model unit="millimeter" xml:lang="en-US" xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">\n'
             f'<metadata name="Title">{name}</metadata>\n<resources>\n<object id="1" type="model" name="{name}">\n'
             f'<mesh>\n<vertices>\n{verts}\n</vertices>\n<triangles>\n{tris}\n</triangles>\n</mesh>\n</object>\n'
             '</resources>\n<build>\n<item objectid="1"/>\n</build>\n</model>\n')
    ct = ('<?xml version="1.0" encoding="UTF-8"?>\n<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
          '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
          '<Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/></Types>')
    rels = ('<?xml version="1.0" encoding="UTF-8"?>\n<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Target="/3D/3dmodel.model" Id="rel0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/></Relationships>')
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", ct)
        z.writestr("_rels/.rels", rels)
        z.writestr("3D/3dmodel.model", model)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("raw")
    ap.add_argument("out")
    ap.add_argument("--target", type=int, default=1_200_000)
    ap.add_argument("--name", default="Creation Ring")
    a = ap.parse_args()
    t0 = time.time()
    v, f = read_ply(a.raw)
    print(f"raw: {len(f)} triangles, open/non-manifold/vol = {stats(v, f)}")
    v, f, dropped = largest_shell(v, f)
    print(f"dropped {dropped} speck shells ({time.time() - t0:.0f}s)")
    mf = pymeshfix.MeshFix(v, f.astype(np.int32))
    mf.repair(joincomp=False, remove_smallest_components=True)
    v, f = np.asarray(mf.points, dtype=np.float64), np.asarray(mf.faces, dtype=np.int64)
    print(f"repaired: {len(f)} triangles, open/non-manifold/vol = {stats(v, f)} ({time.time() - t0:.0f}s)")
    if len(f) > a.target:
        v2, f2 = fast_simplification.simplify(v.astype(np.float32), f.astype(np.int32), 1.0 - a.target / len(f), agg=5)
        v, f = np.asarray(v2, dtype=np.float64), np.asarray(f2, dtype=np.int64)
        print(f"decimated: {len(f)} triangles, open/non-manifold/vol = {stats(v, f)} ({time.time() - t0:.0f}s)")
        mf = pymeshfix.MeshFix(v, f.astype(np.int32))
        mf.repair(joincomp=False, remove_smallest_components=True)
        v, f = np.asarray(mf.points, dtype=np.float64), np.asarray(mf.faces, dtype=np.int64)
        print(f"final: {len(f)} triangles, open/non-manifold/vol = {stats(v, f)} ({time.time() - t0:.0f}s)")
    write_stl(a.out + ".stl", v, f, f"{a.name}, sculpted SDF, mm")
    write_ply(a.out + ".ply", v, f)
    write_3mf(a.out + ".3mf", v, f, a.name)
    print(f"wrote {a.out}.stl/.3mf/.ply ({time.time() - t0:.0f}s)")


main()
