// Human right hand in its local frame: wrist at origin, fingers along +X,
// thumb toward +Y, back of the hand +Z, fingers curl toward -Z.
// Proportions and pose follow ../../hands.scad (everything is a fraction of
// the hand length L); all pose math is done here in JS and emitted as literals.
import { P, f, vec3, fingerChain, rotXYZ, add, mul, matVec } from './params.mjs';

// column j of a row-major 3x3 (the basis vector of local axis j)
const col = (R, j) => [R[0][j], R[1][j], R[2][j]];

export function glsl(L) {
  const s = L, ft = P.fingerThickness;
  // tucked-finger flexion: hands.scad's curl numbers are scaled so the default
  // fingerCurl gives a relaxed, dangling curl rather than a fist
  const c = P.fingerCurl * 0.70;
  const pl = 0.47 * s, pw = 0.42 * s, pt = 0.18 * s, ww = 0.26 * s, wt = 0.17 * s;
  const fy = [0.36, 0.12, -0.12, -0.36].map((k) => k * pw);

  const lines = [];
  const emit = (t) => lines.push('  ' + t);

  // --- forearm and palm --------------------------------------------------------
  // forearm: band-sized oval section at the back, thinning toward the wrist
  const foreA = -0.45 * s + 1.45, foreB = 0.04 * s;
  emit(`float fore = hh_body(q, ${f(foreA)}, ${f(foreB)}, vec2(1.42, 1.50), vec2(1.22, 1.10), 1.45, 0.90);`);
  // palm: wrist oval hulled to the knuckle oval
  const wr = [ww / 2, wt / 2], kr = [pw / 2, pt / 2];
  emit(`float palm = hh_body(q, 0.0, ${f(pl)}, vec2(${f(wr[0])}, ${f(wr[1])}), vec2(${f(kr[0])}, ${f(kr[1])}), ${f(0.12 * s)}, ${f(0.09 * s)});`);
  // metacarpal dome on the back of the hand
  const dome = { c: [pl * 0.60, 0, pt * 0.18], r: [pl * 0.42, pw * 0.36, pt * 0.42] };
  emit(`float dome = sd_ellipsoid(q, ${vec3(dome.c)}, ${vec3(dome.r)});`);
  // thenar pad under the thumb side, hypothenar pad under the pinky side
  emit(`float thenar = sd_ellipsoid(q, ${vec3([0.24 * s, pw * 0.28, -pt * 0.25])}, ${vec3([0.15 * s, pw * 0.20, pt * 0.40])});`);
  emit(`float hypo = sd_ellipsoid(q, ${vec3([0.24 * s, -pw * 0.30, -pt * 0.18])}, ${vec3([0.16 * s, pw * 0.15, pt * 0.34])});`);
  emit(`float d = implicit_union_round(fore, palm, 0.90);`);
  emit(`d = implicit_union_round(d, dome, 0.45);`);
  emit(`d = implicit_union_round(d, thenar, 0.45);`);
  emit(`d = implicit_union_round(d, hypo, 0.40);`);

  // --- extensor tendons: fine ridges just under the back surface ---------------
  const backZ = (x, y) => {
    const t = Math.min(1, Math.max(0, x / pl));
    const ry = wr[0] + (kr[0] - wr[0]) * t, rz = wr[1] + (kr[1] - wr[1]) * t;
    const body = rz * Math.sqrt(Math.max(0, 1 - (y / ry) ** 2));
    const k = 1 - ((x - dome.c[0]) / dome.r[0]) ** 2 - (y / dome.r[1]) ** 2;
    const dz = dome.c[2] + dome.r[2] * Math.sqrt(Math.max(0, k));
    return Math.max(body, dz);
  };
  const tr = 0.017 * s, sink = 0.009 * s;
  let ti = 0;
  for (const y of fy) {
    for (const sg of [[0.18, 0.55, 0.30, 0.80], [0.30, 0.80, 0.44, 1.0]]) {
      const a = [sg[0] * s, y * sg[1], 0], b = [sg[2] * s, y * sg[3], 0];
      a[2] = backZ(a[0], a[1]) - sink; b[2] = backZ(b[0], b[1]) - sink;
      emit(`float tn${ti} = implicit_capsule(q, ${vec3(a)}, ${vec3(b)}, ${f(tr)});`);
      emit(`d = implicit_union_round(d, tn${ti}, 0.12);`);
      ti++;
    }
  }

  // --- fingers -----------------------------------------------------------------
  // emits one finger as blended cone capsules with knuckle swellings and a tip
  // pad; returns the GLSL variable holding the finger's field
  const finger = (name, base, R0, lens, radii, curl, opt = {}) => {
    const r = radii.map((k) => k * s * ft), len = lens.map((k) => k * s);
    const { segs, joints } = fingerChain(base, R0, len, r, curl);
    segs.forEach((sg, i) => emit(`float ${name}${i} = implicit_cone_capsule(q, ${vec3(sg.a)}, ${vec3(sg.b)}, ${f(sg.ra)}, ${f(sg.rb)});`));
    let e = `${name}0`;
    for (let i = 1; i < segs.length; i++) e = `implicit_union_round(${e}, ${name}${i}, 0.30)`;
    emit(`float ${name} = ${e};`);
    // knuckle swellings on the back of each interphalangeal joint
    for (let i = 1; i < segs.length; i++) {
      const z = col(segs[i].R, 2);
      const cpos = add(joints[i], mul(z, 0.16 * r[i]));
      emit(`${name} = implicit_union_round(${name}, implicit_sphere(q, ${vec3(cpos)}, ${f(r[i] * 0.98)}), 0.22);`);
    }
    // fleshy pad under the tip
    const last = segs[segs.length - 1], rt = r[r.length - 1];
    const tip = joints[joints.length - 1];
    const pad = add(tip, add(mul(col(last.R, 0), 0.20 * rt), mul(col(last.R, 2), -0.12 * rt)));
    emit(`${name} = implicit_union_round(${name}, implicit_sphere(q, ${vec3(pad)}, ${f(rt * 0.86)}), 0.20);`);
    // nail: a flat ellipsoid in the last phalanx's frame
    if (opt.nail) {
      const R = last.R;
      const nc = add(tip, add(mul(col(R, 0), -0.45 * rt), mul(col(R, 2), 0.58 * rt)));
      emit(`vec3 ${name}nd = q - ${vec3(nc)};`);
      emit(`vec3 ${name}nq = vec3(dot(${name}nd, ${vec3(col(R, 0))}), dot(${name}nd, ${vec3(col(R, 1))}), dot(${name}nd, ${vec3(col(R, 2))}));`);
      emit(`float ${name}nail = sd_ellipsoid(${name}nq, vec3(0.0, 0.0, 0.0), vec3(${f(1.25 * rt)}, ${f(0.82 * rt)}, ${f(0.26 * rt)}));`);
      emit(`${name} = implicit_union_round(${name}, ${name}nail, 0.08);`);
    }
    return { joints, segs, r };
  };

  const I = [1, 0, 0], yaw = (dg) => rotXYZ(0, 0, dg);
  // index: extended, pointing at the other hand with a slight droop
  finger('idx', [pl, fy[0], 0], yaw(3), [0.24, 0.16, 0.13], [0.056, 0.051, 0.046, 0.040], [-2, 6, 10], { nail: true });
  // middle, ring, pinky: relaxed, hanging downward
  finger('mid', [pl, fy[1], 0.005 * s], yaw(1), [0.26, 0.17, 0.13], [0.056, 0.051, 0.046, 0.040], [40 * c, 55 * c, 45 * c]);
  finger('rng', [pl - 0.01 * s, fy[2], 0], yaw(-4), [0.24, 0.16, 0.12], [0.052, 0.048, 0.043, 0.037], [48 * c, 62 * c, 48 * c]);
  finger('pnk', [pl - 0.04 * s, fy[3], -0.01 * s], yaw(-10), [0.19, 0.13, 0.10], [0.046, 0.042, 0.037, 0.032], [55 * c, 68 * c, 50 * c]);
  // thumb: out to the side, tucking under and forward toward the index
  const thumbR0 = ((A, B) => A.map((row, i) => [0, 1, 2].map((j) => row[0] * B[0][j] + row[1] * B[1][j] + row[2] * B[2][j])))(rotXYZ(0, 35, 38), rotXYZ(-65, 0, 0));
  finger('thb', [0.24 * s, pw * 0.44, -pt * 0.30], thumbR0, [0.20, 0.15], [0.072, 0.060, 0.048], [20, 38], { nail: true });

  // metacarpal knuckles standing proud where the fingers leave the palm
  const kn = [[pl + 0.02 * s, fy[0], 0.18], [pl + 0.04 * s, fy[1], 0.30], [pl + 0.02 * s, fy[2], 0.26], [pl - 0.02 * s, fy[3], 0.14]];
  const knr = [0.056, 0.056, 0.052, 0.046].map((k) => k * s * ft * 1.02);
  kn.forEach((p, i) => emit(`float kn${i} = implicit_sphere(q, ${vec3(p)}, ${f(knr[i])});`));

  // the three hanging fingers stay separate (small blend between them), then
  // everything joins the palm with a fleshy blend
  emit(`float hang = implicit_union_round(implicit_union_round(mid, rng, 0.10), pnk, 0.10);`);
  emit(`float fingers = implicit_union_round(idx, hang, 0.10);`);
  emit(`float knuckles = implicit_union_round(implicit_union_round(implicit_union_round(kn0, kn1, 0.25), kn2, 0.25), kn3, 0.25);`);
  emit(`d = implicit_union_round(d, knuckles, 0.35);`);
  emit(`d = implicit_union_round(d, fingers, 0.38);`);
  emit(`d = implicit_union_round(d, thb, 0.50);`);
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
