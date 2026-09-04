// Robot left hand in its local frame (same frame as the human hand; the
// placement mirrors it, so it is built exactly like a right hand here):
// wrist at origin, fingers along +X, thumb toward +Y, back of the hand +Z,
// fingers curl toward -Z.  Proportions and pose numbers follow
// robot_hand() in ../../hands.scad; every element is a body of revolution
// or a smooth blend so the casting stays curvy.
//
//   forearm   band-section cone capsules along the band's centreline, x >= -0.45 L
//   palm      four ellipsoids blended round (wrist, mid, knuckle end, back dome)
//   fingers   fingerChain() -> per segment: ball knuckle, tapered tube,
//             two raised ring bands (short coaxial cylinders), two rivet
//             domes on the back, rounded tip pad on the last segment
//   thumb     same chain on a rounded pivot barrel at the +Y side
//
// Each finger sits behind a bounding-sphere early-out and the ring/rivet
// detail behind a per-segment tube-distance test: far from the finger the
// field is the (conservative, smaller) bound, so the mesher and marcher only
// pay for the detail near the surface.
import { P, f, vec3, fingerChain, rotXYZ, add, sub, mul, matMul, norm, dot, handFrame, Rc, thEnd, deg } from './params.mjs';

const col = (R, j) => [R[0][j], R[1][j], R[2][j]];

export function glsl(L) {
  const s = L, ft = P.fingerThickness * 1.25, c = P.fingerCurl * 0.85;   // a looser fist than the human's curl so the segments stay legible
  const pl = 0.46 * s, pw = 0.44 * s, pt = 0.19 * s, ww = 0.32 * s, wt = 0.24 * s;
  const gw = 0.018 * s;                    // seam width (hands.scad)
  const kt = 0.94, kb = 1.02;              // tube / ball radius factors: joints read as subtle balls, not marbles
  const bandH = 0.02 * s;                  // how far a ring band stands proud of the tube (about 0.17 mm)
  const bandHalf = gw * 1.1;               // ring band half-width along the segment
  const rivetR = gw * 1.4;                 // rivet dome radius
  const rMachined = 0.12, rRing = 0.04, rRivet = 0.05, rFinger = 0.05;
  const boundMargin = 0.7, detailMargin = 0.5;

  const out = [];
  // ---- one mechanical finger --------------------------------------------
  function finger(id, chain) {
    const { segs, joints } = chain;
    // bounding sphere around every joint plus the tip pad and the rivets
    const cen = mul(joints.reduce((a, b) => add(a, b)), 1 / joints.length);
    let R = 0;
    segs.forEach((sg, i) => {
      R = Math.max(R, norm(sub(sg.a, cen)) + sg.ra * kb + 2 * rivetR + bandH);
      R = Math.max(R, norm(sub(sg.b, cen)) + sg.rb * (i === segs.length - 1 ? 1.2 : kb) + 2 * rivetR + bandH);
    });
    out.push(`  float ${id} = implicit_sphere(q, ${vec3(cen)}, ${f(R)});`);
    out.push(`  if (${id} < ${f(boundMargin)}) {`);
    const d = `d${id}`;
    let first = true;
    const acc = (expr, r, indent = '    ') => {
      if (first) { out.push(`${indent}float ${d} = ${expr};`); first = false; }
      else out.push(`${indent}${d} = implicit_union_round(${d}, ${expr}, ${f(r)});`);
    };
    segs.forEach((sg, i) => {
      const { a, b, ra, rb, R } = sg;
      const ex = col(R, 0), ey = col(R, 1), ez = col(R, 2);
      const tube = `s${id}${i}`;
      acc(`implicit_sphere(q, ${vec3(a)}, ${f(ra * kb)})`, rMachined);                       // ball knuckle
      out.push(`    float ${tube} = implicit_cone_capsule(q, ${vec3(a)}, ${vec3(b)}, ${f(ra * kt)}, ${f(rb * kt)});`);
      acc(tube, rMachined);                                                                    // tapered tube
      out.push(`    if (${tube} < ${f(detailMargin)}) {`);
      [0.32, 0.72].forEach((fr, k) => {
        const cpt = add(a, mul(sub(b, a), fr));
        const rt = (ra + (rb - ra) * fr) * kt;
        // raised ring band: a short capped cylinder coaxial with the segment
        acc(`implicit_cylinder_capped(q, ${vec3(sub(cpt, mul(ex, bandHalf)))}, ${vec3(add(cpt, mul(ex, bandHalf)))}, ${f(rt + bandH)})`, rRing, '      ');
        // rivet dome on the back of the band
        const rc = add(cpt, mul(ez, rt + bandH - rivetR * 0.35));
        acc(`implicit_sphere(q, ${vec3(rc)}, ${f(rivetR)})`, rRivet, '      ');
      });
      out.push(`    }`);
      if (i === segs.length - 1) {                                                        // rounded tip pad
        const tipEnd = add(add(b, mul(ex, 0.65 * rb)), mul(ez, -0.1 * rb));
        acc(`implicit_cone_capsule(q, ${vec3(b)}, ${vec3(tipEnd)}, ${f(rb * kt)}, ${f(rb * 0.5)})`, rMachined);
      }
    });
    out.push(`    ${id} = ${d};`);
    out.push(`  }`);
  }

  // ---- forearm + palm -----------------------------------------------------
  // forearm: the band's own oval section (1.36 side x 1.46 tall, so the band's end
  // cap at the origin is swallowed) laid along the band's centreline expressed in
  // this hand's frame (handFrame(-1) from params, so it follows the band whatever
  // wristBend/gap say), as two cone capsules; evaluated in a Z-compressed space,
  // which keeps the field conservative. Its rounded far end stops at x = -0.45 L.
  const rF = 1.36, kz = 1.36 / 1.46;
  const frame = handFrame(-1);
  const bandLocal = (arc) => {
    const th = deg(thEnd) - arc / Rc;
    const d = sub([Rc * Math.cos(th), Rc * Math.sin(th), 0], frame.origin);
    return frame.axes.map((ax) => dot(d, ax));
  };
  // far end: as far back as x = -0.45 L allows, staying inside the hand-only
  // review bounds (|y| < 0.5 L) so the test renders show no clipped face
  let arcFar = 0.5;
  const inside = (arc) => { const b = bandLocal(arc); return b[0] - rF > -0.45 * s && Math.abs(b[1]) + rF < 0.49 * s; };
  while (arcFar < 12 && inside(arcFar + 0.05)) arcFar += 0.05;
  const A1 = bandLocal(arcFar * 0.5), A2 = bandLocal(arcFar);
  const zs = (p) => [p[0], p[1], p[2] * kz];
  out.push(`  vec3 qa = vec3(q.x, q.y, q.z * ${f(kz)});`);
  out.push(`  float arm = implicit_cone_capsule(qa, ${vec3(zs(A2))}, ${vec3(zs(A1))}, ${f(rF)}, ${f(rF)});`);
  out.push(`  arm = implicit_union_round(arm, implicit_cone_capsule(qa, ${vec3(zs(A1))}, vec3(0.0, 0.0, 0.0), ${f(rF)}, ${f(rF)}), 0.3);`);
  out.push(`  float palm = sd_ellipsoid(q, ${vec3([0.06 * s, 0, 0])}, ${vec3([0.22 * s, ww / 2, wt / 2])});`);
  out.push(`  palm = implicit_union_round(palm, sd_ellipsoid(q, ${vec3([pl * 0.50, 0, 0.01 * s])}, ${vec3([pl * 0.50, (ww + pw) / 4 * 1.03, (wt + pt) / 4])}), 0.45);`);
  out.push(`  palm = implicit_union_round(palm, sd_ellipsoid(q, ${vec3([pl - 0.14 * s, 0, 0])}, ${vec3([0.14 * s, pw / 2, pt / 2])}), 0.45);`);
  // metacarpal dome on the back
  out.push(`  palm = implicit_union_round(palm, sd_ellipsoid(q, ${vec3([pl * 0.56, 0, pt * 0.22])}, ${vec3([pl * 0.48, pw * 0.42, pt * 0.60])}), 0.45);`);
  out.push(`  float body = implicit_union_round(arm, palm, 0.5);`);

  // ---- fingers ------------------------------------------------------------
  const fy = [0.37, 0.125, -0.125, -0.37].map((v) => v * pw);   // hands.scad spacing, opened 3%
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
  const pivHalf = 0.118 * s, pivR = 0.052 * s;
  out.push(`  float pivot = implicit_capsule(q, ${vec3(sub(T, mul(pivAxis, pivHalf)))}, ${vec3(add(T, mul(pivAxis, pivHalf)))}, ${f(pivR)});`);
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
