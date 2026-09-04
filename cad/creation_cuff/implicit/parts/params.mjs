// Shared dimensions and pose for the implicit Creation Ring (mm).
// Same conventions as ../../creation_cuff.scad: ring in the XY plane, Z is the
// ring axis, the opening faces -Y, the robot hand is on -X and the human on +X.
export const P = {
  innerD: 17.3,        // inside diameter at the back (US 7)
  bandT: 2.7,          // radial thickness of the band
  bandW: 2.9,          // axial width of the band
  gapDeg: 116,         // opening between the band ends
  tipGap: 0.8,         // space between the two index fingertips
  wristBend: 0.7,      // 0 = hands continue the curve, 1 = they point straight across
  handPitch: 8,        // degrees the hands droop below the band plane
  fingerThickness: 1.15,
  fingerCurl: 1.4,
  hammer: { depth: 0.2, cell: 1.25, seed: 3 },
};

export const deg = (d) => (d * Math.PI) / 180;
export const Ri = P.innerD / 2;
export const rx = P.bandT / 2, rz = P.bandW / 2;
export const Rc = Ri + rx;                              // centreline radius
export const thStart = -90 + P.gapDeg / 2;              // human end (x > 0)
export const thEnd = 270 - P.gapDeg / 2;                // robot end (x < 0)

// --- tiny vector / matrix helpers (row-major 3x3, columns are basis vectors)
export const v = (x, y, z) => [x, y, z];
export const add = (a, b) => [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
export const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
export const mul = (a, s) => [a[0] * s, a[1] * s, a[2] * s];
export const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
export const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
export const norm = (a) => Math.hypot(a[0], a[1], a[2]);
export const unit = (a) => mul(a, 1 / norm(a));
export const matMul = (A, B) => A.map((r, i) => [0, 1, 2].map((j) => r[0] * B[0][j] + r[1] * B[1][j] + r[2] * B[2][j]));
export const matVec = (A, x) => A.map((r) => dot(r, x));
export const rotX = (a) => [[1, 0, 0], [0, Math.cos(a), -Math.sin(a)], [0, Math.sin(a), Math.cos(a)]];
export const rotY = (a) => [[Math.cos(a), 0, Math.sin(a)], [0, 1, 0], [-Math.sin(a), 0, Math.cos(a)]];
export const rotZ = (a) => [[Math.cos(a), -Math.sin(a), 0], [Math.sin(a), Math.cos(a), 0], [0, 0, 1]];
// OpenSCAD rotate([a,b,c]) == Rz(c)·Ry(b)·Rx(a)
export const rotXYZ = (a, b, c) => matMul(rotZ(deg(c)), matMul(rotY(deg(b)), rotX(deg(a))));
export const f = (x) => (Math.round(x * 10000) / 10000).toFixed(4);
export const vec3 = (p) => `vec3(${f(p[0])}, ${f(p[1])}, ${f(p[2])})`;

// --- hand placement (mirrors creation_cuff.scad's tangent-bent heading)
function heading(side) {
  const t = side > 0 ? thStart - 90 : thEnd + 90;           // tangent yaw toward the gap
  const c = side > 0 ? 180 : 0;                             // chord yaw
  const d = ((c - t + 540) % 360) - 180;
  return deg(t + P.wristBend * d);
}
export function endPoint(side) {
  const th = deg(side > 0 ? thStart : thEnd);
  return [Rc * Math.cos(th), Rc * Math.sin(th), 0];
}
export function handDir(side) { const h = heading(side); return [Math.cos(h), Math.sin(h), 0]; }
export function handLength(side) {
  const e = endPoint(side), d = handDir(side);
  return (side * P.tipGap / 2 - e[0]) / d[0];
}
// World-from-local frame: columns X (fingers), Y (thumb side), Z (back of hand).
// side +1 = human (right hand), -1 = robot (left hand, mirrored so the thumb
// faces the viewer at -Y). Includes the 8° droop.
export function handFrame(side) {
  const x0 = handDir(side);
  let y0 = cross([0, 0, 1], x0);
  if (side < 0) y0 = mul(y0, -1);
  const z0 = [0, 0, 1];
  const pitch = deg(P.handPitch);
  const x = add(mul(x0, Math.cos(pitch)), mul(z0, -Math.sin(pitch)));
  const z = add(mul(x0, Math.sin(pitch)), mul(z0, Math.cos(pitch)));
  return { origin: endPoint(side), axes: [x, y0, z], L: handLength(side) };
}
// GLSL: world point p -> hand-local q (transpose of the axis matrix)
export function frameToLocalGLSL(name, frame) {
  const [x, y, z] = frame.axes, o = frame.origin;
  return `vec3 ${name}(vec3 p) {
  vec3 d = p - ${vec3(o)};
  return vec3(dot(d, ${vec3(x)}), dot(d, ${vec3(y)}), dot(d, ${vec3(z)}));
}`;
}

// --- finger chain: base point, base rotation matrix, phalanx lengths, joint radii
// (len(L)+1), curl per joint in degrees (positive folds toward -Z). Returns
// segments [{a, b, ra, rb}] in the hand frame plus the joint points.
export function fingerChain(base, R0, L, r, curl) {
  const segs = [], joints = [base];
  let R = R0, p = base;
  for (let i = 0; i < L.length; i++) {
    R = matMul(R, rotY(deg(curl[i])));
    const q = add(p, matVec(R, [L[i], 0, 0]));
    segs.push({ a: p, b: q, ra: r[i], rb: r[i + 1], R });
    joints.push(q);
    p = q;
  }
  return { segs, joints, R };
}
