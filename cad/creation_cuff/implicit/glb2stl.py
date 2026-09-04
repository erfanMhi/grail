#!/usr/bin/env python3
"""Convert the plain GLB written by the implicit-cad gen tool into a binary STL.

    python3 glb2stl.py model.glb out.stl
"""
import json, struct, sys

src, dst = sys.argv[1], sys.argv[2]
raw = open(src, "rb").read()
assert raw[:4] == b"glTF", "not a GLB"
clen, _ = struct.unpack_from("<II", raw, 12)
js = json.loads(raw[20:20 + clen])
boff = 20 + clen
blen, btype = struct.unpack_from("<II", raw, boff)
bin_ = raw[boff + 8: boff + 8 + blen]

def accessor(idx):
    a = js["accessors"][idx]
    bv = js["bufferViews"][a["bufferView"]]
    off = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
    comp = {5120: "b", 5121: "B", 5122: "h", 5123: "H", 5125: "I", 5126: "f"}[a["componentType"]]
    n = {"SCALAR": 1, "VEC2": 2, "VEC3": 3}[a["type"]]
    vals = struct.unpack_from("<%d%s" % (a["count"] * n, comp), bin_, off)
    return [vals[i:i + n] for i in range(0, len(vals), n)] if n > 1 else list(vals)

tris = []
for mesh in js["meshes"]:
    for prim in mesh["primitives"]:
        pos = accessor(prim["attributes"]["POSITION"])
        idx = accessor(prim["indices"]) if "indices" in prim else list(range(len(pos)))
        for i in range(0, len(idx), 3):
            tris.append((pos[idx[i]], pos[idx[i + 1]], pos[idx[i + 2]]))

with open(dst, "wb") as out:
    out.write(b"\0" * 80 + struct.pack("<I", len(tris)))
    for a, b, c in tris:
        ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
        vx, vy, vz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
        nx, ny, nz = uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx
        l = (nx * nx + ny * ny + nz * nz) ** 0.5 or 1.0
        out.write(struct.pack("<12fH", nx / l, ny / l, nz / l, *a, *b, *c, 0))
print(f"{dst}: {len(tris)} triangles")
