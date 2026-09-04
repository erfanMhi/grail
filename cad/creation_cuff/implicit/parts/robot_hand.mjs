// Robot left hand in its local frame (same frame as the human hand; the
// placement mirrors it, so it is built exactly like a right hand here):
// wrist at origin, fingers along +X, thumb toward +Y, back of the hand +Z,
// fingers curl toward -Z.  Proportions and pose numbers follow
// robot_hand() in ../../hands.scad; every element is a body of revolution
// or a smooth blend so the casting stays curvy.
//
//   forearm   cone capsule from x = -0.45 L tapering into the wrist
//   palm      four ellipsoids blended round (wrist, mid, knuckle end, back dome)
//   fingers   fingerChain() -> per segment: ball knuckle, tapered tube,
//             two raised ring bands (torus in the segment frame), two rivet
//             domes on the back, rounded tip pad on the last segment
//   thumb     same chain on a rounded pivot barrel at the +Y side
import { P, f, vec3, fingerChain, rotXYZ, add, sub, mul, matMul } from './params.mjs';

const col = (R, j) => [R[0][j], R[1][j], R[2][j]];

export function glsl(L) {
  const s = L, ft = P.fingerThickness * 1.25, c = P.fingerCurl;
  const pl = 0.46 * s, pw = 0.44 * s, pt = 0.19 * s, ww = 0.32 * s, wt = 0.24 * s;
  const gw = 0.018 * s;                    // seam width (hands.scad)
  const kt = 0.94, kb = 1.04;              // tube / ball radius factors (hands.scad)
  const bandH = 0.008 * s;                 // how far a ring band stands proud of the tube
  const bandMr = 0.020 * s;                // ring band torus minor radius
  const rivetR = gw * 1.3;                 // rivet dome radius
  const rMachined = 0.12, rRing = 0.06, rRivet = 0.05, rFinger = 0.05;

  const out = [];
  // ---- one mechanical finger --------------------------------------------
  function finger(id, chain) {
    let first = true;
    const acc = (expr, r) => {
      if (first) { out.push(`  float ${id} = ${expr};`); first = false; }
      else out.push(`  ${id} = implicit_union_round(${id}, ${expr}, ${f(r)});`);
    };
    chain.segs.forEach((sg, i) => {
      const { a, b, ra, rb, R } = sg;
      const ex = col(R, 0), ey = col(R, 1), ez = col(R, 2);
      acc(`implicit_sphere(q, ${vec3(a)}, ${f(ra * kb)})`, rMachined);                       // ball knuckle
      acc(`implicit_cone_capsule(q, ${vec3(a)}, ${vec3(b)}, ${f(ra * kt)}, ${f(rb * kt)})`, rMachined); // tube
      [0.32, 0.72].forEach((fr, k) => {
        const cpt = add(a, mul(sub(b, a), fr));
        const rt = (ra + (rb - ra) * fr) * kt;
        const tn = `t${id}${i}${k}`;
        out.push(`  vec3 ${tn} = q - ${vec3(cpt)};`);
        // raised ring band: torus around the segment axis (local z = axis)
        acc(`implicit_torus(vec3(dot(${tn}, ${vec3(ey)}), dot(${tn}, ${vec3(ez)}), dot(${tn}, ${vec3(ex)})), ${f(rt + bandH - bandMr)}, ${f(bandMr)})`, rRing);
        // rivet dome on the back of the band
        const rc = add(cpt, mul(ez, rt + bandH - rivetR * 0.35));
        acc(`implicit_sphere(q, ${vec3(rc)}, ${f(rivetR)})`, rRivet);
      });
      if (i === chain.segs.length - 1) {                                                        // rounded tip pad
        const tipEnd = add(add(b, mul(ex, 0.65 * rb)), mul(ez, -0.1 * rb));
        acc(`implicit_cone_capsule(q, ${vec3(b)}, ${vec3(tipEnd)}, ${f(rb * kt)}, ${f(rb * 0.5)})`, rMachined);
      }
    });
  }

  // ---- forearm + palm -----------------------------------------------------
  const F = [-0.45 * s, -0.06 * s, -0.02 * s], W = [-0.05 * s, 0, 0];
  out.push(`  float arm = implicit_cone_capsule(q, ${vec3(F)}, ${vec3(W)}, ${f(1.42)}, ${f(1.28)});`);
  out.push(`  float palm = sd_ellipsoid(q, ${vec3([0.10 * s, 0, 0])}, ${vec3([0.26 * s, ww / 2, wt / 2])});`);
  out.push(`  palm = implicit_union_round(palm, sd_ellipsoid(q, ${vec3([pl * 0.50, 0, 0.01 * s])}, ${vec3([pl * 0.50, (ww + pw) / 4 * 1.03, (wt + pt) / 4])}), 0.45);`);
  out.push(`  palm = implicit_union_round(palm, sd_ellipsoid(q, ${vec3([pl - 0.14 * s, 0, 0])}, ${vec3([0.14 * s, pw / 2, pt / 2])}), 0.45);`);
  // metacarpal dome on the back
  out.push(`  palm = implicit_union_round(palm, sd_ellipsoid(q, ${vec3([pl * 0.56, 0, pt * 0.20])}, ${vec3([pl * 0.46, pw * 0.42, pt * 0.48])}), 0.4);`);
  out.push(`  float body = implicit_union_round(arm, palm, 0.6);`);

  // ---- fingers ------------------------------------------------------------
  const fy = [0.36, 0.12, -0.12, -0.36].map((v) => v * pw);
  const rIdx = [0.058, 0.052, 0.046, 0.038].map((v) => v * s * ft);
  // index: extended; phalanges scaled so the tip pad lands at x = L
  const idxLen = [0.24, 0.16, 0.13].map((v) => v * s);
  const kIdx = (s - pl - 1.15 * rIdx[3]) / (idxLen[0] + idxLen[1] + idxLen[2]);
  const index = fingerChain([pl, fy[0], 0], rotXYZ(0, 0, 3), idxLen.map((v) => v * kIdx), rIdx, [-2, 4, 6]);
  const middle = fingerChain([pl, fy[1], 0], rotXYZ(0, 0, 0), [0.26, 0.17, 0.13].map((v) => v * s), rIdx, [40, 58, 45].map((v) => v * c));
  const ring = fingerChain([pl - 0.01 * s, fy[2], 0], rotXYZ(0, 0, -3), [0.24, 0.16, 0.12].map((v) => v * s), [0.054, 0.049, 0.043, 0.036].map((v) => v * s * ft), [48, 64, 48].map((v) => v * c));
  const pinky = fingerChain([pl - 0.04 * s, fy[3], -0.005 * s], rotXYZ(0, 0, -8), [0.19, 0.13, 0.10].map((v) => v * s), [0.048, 0.043, 0.038, 0.032].map((v) => v * s * ft), [55, 70, 50].map((v) => v * c));
  finger('fI', index);
  finger('fM', middle);
  finger('fR', ring);
  finger('fP', pinky);
  out.push(`  float fingers = implicit_union_round(fI, fM, ${f(rFinger)});`);
  out.push(`  fingers = implicit_union_round(fingers, fR, ${f(rFinger)});`);
  out.push(`  fingers = implicit_union_round(fingers, fP, ${f(rFinger)});`);
  out.push(`  float hand = implicit_union_round(body, fingers, 0.15);`);

  // ---- thumb on a rounded pivot barrel at the +Y side ---------------------
  const T = [0.20 * s, pw * 0.46, -pt * 0.15];
  const pivAxis = [Math.sin(Math.PI * 50 / 180), -Math.cos(Math.PI * 50 / 180), 0];   // rotate([0,0,50]) rotate([90,0,0]) Z
  const pivHalf = 0.10 * s, pivR = 0.068 * s;
  out.push(`  float pivot = implicit_capsule(q, ${vec3(sub(T, mul(pivAxis, pivHalf)))}, ${vec3(add(T, mul(pivAxis, pivHalf)))}, ${f(pivR)});`);
  out.push(`  pivot = implicit_union_round(pivot, implicit_sphere(q, ${vec3(T)}, ${f(0.078 * s)}), 0.1);`);
  const R0 = matMul(rotXYZ(0, 20, 50), rotXYZ(-45, 0, 0));
  const thumb = fingerChain(T, R0, [0.21, 0.15].map((v) => v * s), [0.072, 0.062, 0.050].map((v) => v * s * ft), [15, 32].map((v) => v * c));
  finger('fT', thumb);
  out.push(`  hand = implicit_union_round(hand, pivot, 0.15);`);
  out.push(`  hand = implicit_union_round(hand, fT, ${f(rMachined)});`);
  out.push(`  return hand;`);

  return `
// --- robot hand -------------------------------------------------------------
float robot_hand_sdf(vec3 q) {
${out.join('\n')}
}`;
}
