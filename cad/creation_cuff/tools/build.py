"""Build the silver three.js viewer page: python3 build.py <stl_dir> <out.html> [keys] [KEY=file,...]"""
import base64, struct, sys, os, json
SRC = sys.argv[1] if len(sys.argv) > 1 else 'export'
OUT = sys.argv[2] if len(sys.argv) > 2 else 'creation_cuff_viewer.html'

def stl_vertices(path):
    """Yield (x, y, z) per vertex from an ASCII or binary STL."""
    raw = open(path, 'rb').read()
    if raw[:5] == b'solid' and b'facet' in raw[:2000]:
        for line in raw.decode('ascii', 'ignore').splitlines():
            t = line.split()
            if t and t[0] == 'vertex':
                yield (float(t[1]), float(t[2]), float(t[3]))
    else:
        n = struct.unpack_from('<I', raw, 80)[0]
        off = 84
        for _ in range(n):
            f = struct.unpack_from('<12f', raw, off)
            yield f[3:6]; yield f[6:9]; yield f[9:12]
            off += 50

def ascii_to_indexed(path):
    """STL -> deduplicated float32 positions + uint32 triangle indices."""
    verts, idx, pos = {}, [], []
    for v in stl_vertices(path):
        k = (round(v[0], 3), round(v[1], 3), round(v[2], 3))
        i = verts.get(k)
        if i is None:
            i = len(pos) // 3; verts[k] = i; pos.extend(k)
        idx.append(i)
    return struct.pack('<%df' % len(pos), *pos), struct.pack('<%dI' % len(idx), *idx), len(idx) // 3

models = {}
ONLY = sys.argv[3].split(',') if len(sys.argv) > 3 else None
FILES = [('cring', 'creation_ring.stl'), ('bangle', 'creation_cuff_bangle.stl'), ('ring', 'creation_cuff_ring.stl')]
if len(sys.argv) > 4:   # KEY=path overrides
    FILES = [(kv.split('=')[0], kv.split('=')[1]) for kv in sys.argv[4].split(',')]
for key, fn in FILES:
    if ONLY and key not in ONLY: continue
    pb, ib, n = ascii_to_indexed(os.path.join(SRC, fn))
    models[key] = {'pos': base64.b64encode(pb).decode(), 'idx': base64.b64encode(ib).decode(), 'tris': n}
    print(key, n, 'tris', (len(pb) + len(ib)) // 1024, 'KB indexed')

tpl = open(os.path.join(os.path.dirname(__file__), 'template.html')).read()
html = tpl.replace('__MODELS_JSON__', json.dumps(models))
open(OUT, 'w').write(html)
print('wrote', OUT, len(html) // 1024, 'KB')
