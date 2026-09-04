// Band: an oval-section torus arc with planished hammer marks on the outside.
import { P, Ri, rx, rz, Rc, thStart, thEnd, f } from './params.mjs';

export function glsl() {
  const s = P.hammer.seed;
  return `
// --- band -------------------------------------------------------------------
float hash21(vec2 p) { p = fract(p * vec2(123.34, 456.21)); p += dot(p, p + 45.32); return fract(p.x * p.y); }
// distance to an ellipse in 2D (quick, accurate near the boundary)
float ellipse2(vec2 q, vec2 r) { vec2 n = q / r; float k0 = length(n); float k1 = length(n / r); return k0 * (k0 - 1.0) / max(k1, 1e-5); }
// hammer marks: a jittered hex lattice of shallow round dimples in (arc length, z),
// each with its own size and depth, overlapping like planishing blows
float hammer_marks(float arcMM, float z, float fade) {
  vec2 cellSize = vec2(${f(P.hammer.cell)}, ${f(P.hammer.cell * 0.87)});
  // rotate the lattice so its rows never line up with the band's axis or its length
  vec2 sz = vec2(arcMM * 0.9063 - z * 0.4226, arcMM * 0.4226 + z * 0.9063);
  vec2 uv = sz / cellSize;
  float acc = 0.0;
  for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
    vec2 cell = floor(uv) + vec2(float(i), float(j));
    float row = mod(cell.y, 2.0);
    vec2 c = cell + vec2(0.5 + 0.5 * row, 0.5);
    float h1 = hash21(cell + ${f(s)});
    float h2 = hash21(cell + ${f(s)} + 7.0);
    float h3 = hash21(cell + ${f(s)} + 13.0);
    c += (vec2(h1, h2) - 0.5) * 0.9;
    vec2 dv = (uv - c) * cellSize;
    float rd = ${f(P.hammer.cell)} * (0.42 + 0.22 * h3);
    float k = 1.0 - dot(dv, dv) / (rd * rd);
    acc += max(k, 0.0) * max(k, 0.0) * (0.6 + 0.7 * h2);
  }
  return ${f(P.hammer.depth)} * fade * min(acc, 1.3);
}
float band_sdf(vec3 p) {
  float ang = degrees(atan(p.y, p.x));
  float a = ang < ${f(thStart)} ? ang + 360.0 : ang;          // thStart .. thStart+360
  bool onArc = a <= ${f(thEnd)};
  float rr = length(p.xy);
  vec2 q = vec2(rr - ${f(Rc)}, p.z);
  float d;
  if (onArc) {
    d = ellipse2(q, vec2(${f(rx)}, ${f(rz)}));
    float fadeEnds = smoothstep(0.0, 22.0, a - ${f(thStart)}) * smoothstep(0.0, 22.0, ${f(thEnd)} - a);
    float outer = smoothstep(-0.4, 0.4, q.x);                    // only the outer face is planished
    d += hammer_marks(radians(a) * ${f(Rc)}, p.z, fadeEnds * outer);
  } else {
    // in the opening: distance to the nearer end cap
    vec3 c1 = vec3(${f(Rc * Math.cos(thStart * Math.PI / 180))}, ${f(Rc * Math.sin(thStart * Math.PI / 180))}, 0.0);
    vec3 c2 = vec3(${f(Rc * Math.cos(thEnd * Math.PI / 180))}, ${f(Rc * Math.sin(thEnd * Math.PI / 180))}, 0.0);
    vec3 e = vec3(${f(rx)}, ${f(rx)}, ${f(rz)});
    vec3 n1 = (p - c1) / e;
    vec3 n2 = (p - c2) / e;
    float d1 = length(n1) * (length(n1) - 1.0) / max(length(n1 / e), 1e-5);
    float d2 = length(n2) * (length(n2) - 1.0) / max(length(n2 / e), 1e-5);
    d = min(d1, d2);
  }
  return d;
}`;
}
