// Human right hand in its local frame: wrist at origin, fingers along +X,
// thumb toward +Y, back of the hand +Z, fingers curl toward -Z.
// Proportions and pose follow ../../hands.scad (everything is a fraction of
// the hand length L); all pose math is done here in JS and emitted as literals.
import { P, f, vec3, fingerChain, rotXYZ, matMul, add, mul } from './params.mjs';

// column j of a row-major 3x3 (the basis vector of local axis j)
const col = (R, j) => [R[0][j], R[1][j], R[2][j]];

export function glsl(L) {
  const s = L, ft = P.fingerThickness;
  // tucked-finger flexion: the default fingerCurl gives a relaxed, dangling
  // curl (fingers hanging from the knuckles) rather than a fist
  const c = P.fingerCurl / 1.4;
  const pl = 0.47 * s, pw = 0.42 * s, pt = 0.18 * s, ww = 0.26 * s, wt = 0.17 * s;
  const fy = [0.36, 0.12, -0.12, -0.36].map((k) => k * pw);

  const lines = [];
  const emit = (t) => lines.push('  ' + t);

  // --- forearm -----------------------------------------------------------------
  // Band-sized oval section (the band cap is 1.35 x 1.45 at the origin), kept
  // to x = -0.45 L. Its axis is bent a little toward -Y / -Z so that in the
  // assembled ring it hugs the curving band instead of striking out from it.
  const Rf = rotXYZ(0, -8, 14);
  emit(`vec3 qf = vec3(dot(q, ${vec3(col(Rf, 0))}), dot(q, ${vec3(col(Rf, 1))}), dot(q, ${vec3(col(Rf, 2))}));`);
  emit(`float fore = hh_body(qf, ${f(-0.45 * s + 1.40)}, 0.05, vec2(1.30, 1.40), vec2(1.36, 1.44), 1.40, 1.00);`);

  // --- palm: a wedge, band-thick at the wrist thinning to the knuckle row ------
  const px0 = 0.25, wr = [ww / 2 + 0.08, 1.30], kr = [pw / 2, pt / 2];
  emit(`float palm = hh_body(q, ${f(px0)}, ${f(pl)}, vec2(${f(wr[0])}, ${f(wr[1])}), vec2(${f(kr[0])}, ${f(kr[1])}), 1.10, ${f(0.095 * s)});`);
  // metacarpal dome on the back of the hand
  const dome = { c: [pl * 0.68, 0, 0.45], r: [pl * 0.36, pw * 0.34, 0.62] };
  emit(`float dome = sd_ellipsoid(q, ${vec3(dome.c)}, ${vec3(dome.r)});`);
  // thenar pad under the thumb side, hypothenar pad under the pinky side
  emit(`float thenar = sd_ellipsoid(q, ${vec3([0.26 * s, pw * 0.26, -0.55])}, ${vec3([0.16 * s, pw * 0.21, 0.55])});`);
  emit(`float hypo = sd_ellipsoid(q, ${vec3([0.27 * s, -pw * 0.30, -0.50])}, ${vec3([0.17 * s, pw * 0.15, 0.48])});`);
  // head of the ulna: a small bump on the back of the wrist, pinky side
  emit(`float ulna = sd_ellipsoid(q, vec3(0.60, -1.15, 0.50), vec3(0.50, 0.32, 0.30));`);
  emit(`float d = implicit_union_round(fore, palm, 0.70);`);
  emit(`d = implicit_union_round(d, ulna, 0.30);`);
  emit(`d = implicit_union_round(d, dome, 0.45);`);
  emit(`d = implicit_union_round(d, thenar, 0.45);`);
  emit(`d = implicit_union_round(d, hypo, 0.40);`);

  // --- extensor tendons: fine ridges just under the back surface ---------------
  const backZ = (x, y) => {
    const t = Math.min(1, Math.max(0, (x - px0) / (pl - px0)));
    const ry = wr[0] + (kr[0] - wr[0]) * t, rz = wr[1] + (kr[1] - wr[1]) * t;
    const body = rz * Math.sqrt(Math.max(0, 1 - (y / ry) ** 2));
    const k = 1 - ((x - dome.c[0]) / dome.r[0]) ** 2 - (y / dome.r[1]) ** 2;
    const dz = dome.c[2] + dome.r[2] * Math.sqrt(Math.max(0, k));
    return Math.max(body, dz);
  };
  const tr = 0.019 * s, sink = 0.0055 * s;
  let ti = 0;
  for (const y of fy) {
    for (const sg of [[0.20, 0.50, 0.31, 0.78], [0.31, 0.78, 0.45, 1.0]]) {
      const a = [sg[0] * s, y * sg[1], 0], b = [sg[2] * s, y * sg[3], 0];
      a[2] = backZ(a[0], a[1]) - sink; b[2] = backZ(b[0], b[1]) - sink;
      emit(`float tn${ti} = implicit_capsule(q, ${vec3(a)}, ${vec3(b)}, ${f(tr)});`);
      emit(`d = implicit_union_round(d, tn${ti}, 0.10);`);
      ti++;
    }
  }

  // --- fingers -----------------------------------------------------------------
  // One finger = chain of cone capsules (they share their joint spheres, so a
  // small blend only fills the crease inside each bend), a subtle swelling on
  // the back of each joint, a fleshy tip pad and optionally a nail.
  const finger = (name, base, R0, lens, radii, curl, opt = {}) => {
    const r = radii.map((k) => k * s * ft), len = lens.map((k) => k * s);
    const { segs, joints } = fingerChain(base, R0, len, r, curl);
    segs.forEach((sg, i) => emit(`float ${name}${i} = implicit_cone_capsule(q, ${vec3(sg.a)}, ${vec3(sg.b)}, ${f(sg.ra)}, ${f(sg.rb)});`));
    let e = `${name}0`;
    for (let i = 1; i < segs.length; i++) e = `implicit_union_round(${e}, ${name}${i}, 0.12)`;
    emit(`float ${name} = ${e};`);
    for (let i = 1; i < segs.length; i++) {
      const cpos = add(joints[i], mul(col(segs[i].R, 2), 0.18 * r[i]));
      emit(`${name} = implicit_union_round(${name}, implicit_sphere(q, ${vec3(cpos)}, ${f(r[i] * 0.93)}), 0.10);`);
    }
    if (opt.knuckle) emit(`${name} = implicit_union_round(${name}, implicit_sphere(q, ${vec3(opt.knuckle)}, ${f(r[0] * 0.95)}), 0.10);`);
    const last = segs[segs.length - 1], rt = r[r.length - 1], R = last.R;
    const tip = joints[joints.length - 1];
    const pad = add(tip, add(mul(col(R, 0), 0.20 * rt), mul(col(R, 2), -0.12 * rt)));
    emit(`${name} = implicit_union_round(${name}, implicit_sphere(q, ${vec3(pad)}, ${f(rt * 0.85)}), 0.15);`);
    if (opt.nail) {
      const nc = add(tip, add(mul(col(R, 0), -0.40 * rt), mul(col(R, 2), 0.60 * rt)));
      emit(`vec3 ${name}nd = q - ${vec3(nc)};`);
      emit(`vec3 ${name}nq = vec3(dot(${name}nd, ${vec3(col(R, 0))}), dot(${name}nd, ${vec3(col(R, 1))}), dot(${name}nd, ${vec3(col(R, 2))}));`);
      emit(`float ${name}nail = sd_ellipsoid(${name}nq, vec3(0.0, 0.0, 0.0), vec3(${f(1.25 * rt)}, ${f(0.80 * rt)}, ${f(0.26 * rt)}));`);
      emit(`${name} = implicit_union_round(${name}, ${name}nail, 0.06);`);
    }
    return { joints, segs, r };
  };

  const yaw = (dg) => rotXYZ(0, 0, dg);
  const knuckle = (x, y, z) => [x, y, z];
  // index: extended, pointing at the other hand with a slight droop
  finger('idx', [pl, fy[0], 0], yaw(3), [0.24, 0.16, 0.13], [0.056, 0.051, 0.046, 0.040], [-2, 7, 13],
    { nail: true, knuckle: knuckle(pl + 0.10, fy[0], 0.30) });
  // middle, ring, pinky: relaxed, hanging from the knuckles
  finger('mid', [pl, fy[1], 0.005 * s], yaw(0), [0.26, 0.17, 0.13], [0.056, 0.051, 0.046, 0.040], [60 * c, 42 * c, 30 * c],
    { knuckle: knuckle(pl + 0.12, fy[1], 0.38) });
  finger('rng', [pl - 0.01 * s, fy[2], 0], yaw(-5), [0.24, 0.16, 0.12], [0.052, 0.048, 0.043, 0.037], [64 * c, 48 * c, 32 * c],
    { knuckle: knuckle(pl + 0.06, fy[2], 0.35) });
  finger('pnk', [pl - 0.04 * s, fy[3], -0.01 * s], yaw(-12), [0.19, 0.13, 0.10], [0.046, 0.042, 0.037, 0.032], [68 * c, 52 * c, 34 * c],
    { knuckle: knuckle(pl - 0.22, fy[3], 0.24) });
  // thumb: rooted at the +Y side, tucking under and forward toward the index
  const tb = [0.23 * s, pw * 0.38, -pt * 0.35];
  const thumbR0 = matMul(rotXYZ(0, 22, 28), rotXYZ(-75, 0, 0));
  finger('thb', tb, thumbR0, [0.20, 0.15], [0.066, 0.058, 0.048], [18, 50], { nail: true });
  // thumb metacarpal: the thenar mass running from the wrist to the thumb root
  emit(`float thmc = implicit_cone_capsule(q, ${vec3([0.10 * s, pw * 0.22, -0.45])}, ${vec3(tb)}, 0.46, ${f(0.066 * s * ft * 0.92)});`);
  emit(`float thumb = implicit_union_round(thb, thmc, 0.25);`);

  // the three hanging fingers stay separate (small blend between them); then
  // everything joins the palm with a fleshy blend
  emit(`float hang = implicit_union_round(implicit_union_round(mid, rng, 0.10), pnk, 0.10);`);
  emit(`float fingers = implicit_union_round(idx, hang, 0.10);`);
  emit(`d = implicit_union_round(d, fingers, 0.38);`);
  emit(`d = implicit_union_round(d, thumb, 0.45);`);
  emit(`return d;`);

  return `
// --- human hand -------------------------------------------------------------
float sd_ellipsoid(vec3 p, vec3 c, vec3 r) { vec3 n = (p - c) / r; float k0 = length(n); float k1 = length(n / r); return k0 * (k0 - 1.0) / max(k1, 1e-5); }
// tapered body along +X from x0 to x1 with an oval section (half-sizes r0 -> r1
// in Y and Z), ends rounded by ellipsoids of X half-size ex0 / ex1
float hh_body(vec3 p, float x0, float x1, vec2 r0, vec2 r1, float ex0, float ex1) {
  float t = clamp((p.x - x0) / (x1 - x0), 0.0, 1.0);
  vec2 r = mix(r0, r1, t);
  vec2 n = p.yz / r;
  float k0 = length(n);
  float k1 = length(n / r);
  float side = k0 * (k0 - 1.0) / max(k1, 1e-5);
  float d = max(side, max(x0 - p.x, p.x - x1));
  d = min(d, sd_ellipsoid(p, vec3(x0, 0.0, 0.0), vec3(ex0, r0.x, r0.y)));
  d = min(d, sd_ellipsoid(p, vec3(x1, 0.0, 0.0), vec3(ex1, r1.x, r1.y)));
  return d;
}
float human_hand_sdf(vec3 q) {
${lines.join('\n')}
}`;
}
