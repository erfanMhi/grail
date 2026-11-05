// The robot hand: the human hand's skeleton rebuilt as polished machine parts.
// Language: the back of the hand is four long rounded plates fanning from the
// wrist and running straight on into the fingers; each finger is a chain of
// tapered shells with a raised, beaded collar at every joint; the thumb is a
// true cone of three segments ending in a rounded cap. Every groove and bead
// is sized to survive lost-wax casting (grooves >= 0.12 mm wide, beads ~0.2 mm).
#pragma once
#include "common.h"

namespace model {

// a ring of n beads around the local x axis at x = x0
inline float beadRing(vec3 l, float x0, float R, float rb, int n) {
  float ang = std::atan2(l.z, l.y);
  float sec = 2.f * PI / n;
  float a = std::fmod(ang + 100.f * PI + sec * 0.5f, sec) - sec * 0.5f;
  float rho = len2(vec2(l.y, l.z));
  vec3 c = vec3(l.x - x0, rho * std::cos(a) - R, rho * std::sin(a));
  return length(c) - rb;
}

// groove limited to the part of the surface above a plane (z > z0 locally)
inline float grooveAbove(float d, float sheet, float w, float depth, float k, float zl, float z0) {
  float g = std::fmax(std::fabs(sheet) - w, -(d + depth));
  g = std::fmax(g, z0 - zl);
  return smax(d, -g, k);
}

// castable groove dimensions (mm)
constexpr float GW = 0.060f;   // half width
constexpr float GD = 0.075f;   // depth
constexpr float GK = 0.035f;   // edge rounding

// One finger segment. `taper` is how much of the joint-to-joint radius change
// the shell really takes (fingers stay fairly straight, the thumb is a cone).
inline float robotShell(vec3 q, const Bone& b, bool tip, bool first, bool panel, float taper, float edge = 0.16f) {
  vec3 l = b.loc(q);
  const float sq = 0.92f;
  float x0 = (first ? 0.10f : 0.30f) * b.ra;
  float x1 = tip ? b.len : b.len - 0.30f * b.rb;
  float r0 = b.ra * 0.97f, r1 = mix(r0, b.rb * 0.97f, taper);
  vec3 ls = vec3(l.x - x0, l.y, l.z / sq);
  float d;
  if (tip) {
    // a rounded bullet cap with a recessed oval nail panel on its back
    float L = std::fmax(x1 - x0 - 0.30f * r1, 0.1f * b.len);
    d = sdRoundCone(ls, vec3(0.f), vec3(L, 0.f, 0.f), r0, r1 * 0.80f) * sq;
    d = smax(d, -ls.x, 0.10f * r0);
    float xc = L * 0.70f + 0.15f * r1;
    float outline = sdEllipse2(vec2(ls.x - xc, l.y), vec2(0.36f * L + 0.30f * r1, 0.52f * r1));
    float panelMask = sstep(0.05f * r1, 0.35f * r1, l.z);
    d += 0.05f * panelMask * sstep(0.02f, -0.05f, outline);            // recessed plate
    d = grooveAbove(d, outline, GW * 0.8f, GD, GK, l.z, 0.15f * r1);
  } else {
    d = sdTaperCanX(ls, x1 - x0, r0, r1, edge * r0) * sq;
    float xs = x0 + (x1 - x0) * (first ? 0.45f : 0.40f);
    d = groove(d, l.x - xs, GW, GD, GK);
    if (panel) {   // a recessed oval hatch on the back of the big thumb segment
      float xm = x0 + (x1 - x0) * 0.62f;
      float outline = sdEllipse2(vec2(l.x - xm, l.y), vec2(0.26f * (x1 - x0), 0.46f * b.ra));
      float panelMask = sstep(0.15f * b.ra, 0.5f * b.ra, l.z);
      d += 0.055f * panelMask * sstep(0.03f, -0.06f, outline);
      d = grooveAbove(d, outline, GW * 0.8f, GD, GK, l.z, 0.2f * b.ra);
    }
  }
  return d;
}

// Raised collar at a joint: a washer standing proud of both shells with a
// ring of beads half sunk into its rim; set on the bisector of the two bones
// so it reads at any flexion. The joint core fills the gap underneath.
inline float robotCollar(vec3 q, const Bone& prev, const Bone& next, int nb) {
  vec3 ax = normalize(prev.R[0] + next.R[0]);
  mat3 R = frameXZ(ax, prev.R[2] + next.R[2]);
  vec3 l = glm::transpose(R) * (q - next.a);
  float r = next.ra;
  const float sq = 0.92f;
  vec3 ls = vec3(l.x, l.y, l.z / sq);
  float core = sdSphere(l, 0.80f * r);
  float washer = sdRoundCylX(ls + vec3(0.17f * r, 0.f, 0.f), 0.34f * r, 1.02f * r, 0.10f * r) * sq;
  float d = std::fmin(core, washer);
  float bead = beadRing(ls, 0.f, 1.00f * r, 0.19f * r, nb) * sq;
  return smin(d, bead, 0.03f * r);
}

inline float robotFinger(vec3 q, const Chain& c) {
  float d = sdSphere(c.ph[0].loc(q), c.ph[0].ra * 0.80f);    // knuckle core, mostly hidden
  for (int i = 0; i < c.n; i++) {
    d = std::fmin(d, robotShell(q, c.ph[i], i == c.n - 1, i == 0, false, 0.6f));
    if (i > 0) d = std::fmin(d, robotCollar(q, c.ph[i - 1], c.ph[i], 9));
  }
  return d;
}

inline float robotHandLocal(vec3 q) {
  const float H = P.Hr;
  vec3 u = q / H;
  float d = 1e9f;
  // --- the back of the hand: four long plates, domed across, fanning from
  // the wrist and ending over each knuckle at the finger's own gauge, so the
  // top line of the hand runs on unbroken into the index
  for (int i = 0; i < 4; i++) {
    const Bone& b = rF[i + 1].ph[0];
    vec3 a0 = vec3(0.06f, 0.050f - 0.033f * i, 0.018f) * H;
    vec3 a1 = b.a + b.R[0] * (0.10f * b.ra) + b.R[2] * (0.06f * b.ra);
    mat3 Rp = frameXZ(a1 - a0, vec3(0, 0, 1));
    vec3 l = glm::transpose(Rp) * (q - a0);
    float L = length(a1 - a0);
    vec3 ls = vec3(l.x, l.y / 1.22f, l.z / 0.80f);
    float dp = sdRoundCone(ls, vec3(0.f), vec3(L, 0.f, 0.f), 0.048f * H, 0.88f * b.ra) * 0.80f;
    d = (i == 0) ? dp : smin(d, dp, 0.006f * H);
  }
  // palm underneath, and the pod the thumb pivots from
  d = smin(d, sdEllipsoid(u - vec3(0.33f, 0.0f, -0.032f), vec3(0.20f, 0.150f, 0.052f)) * H, 0.02f * H);
  vec3 th = rotZ(rad(-28.f)) * (u - vec3(0.17f, 0.100f, -0.055f));
  d = smin(d, sdEllipsoid(th, vec3(0.12f, 0.070f, 0.064f)) * H, 0.02f * H);
  // short wrist neck that the band runs into, with one seam
  {
    vec3 ln = vec3(q.x, q.y, q.z / 0.74f);
    float neck = sdRoundCone(ln, vec3(-0.05f * H, 0.f, 0.f), vec3(0.09f * H, 0.f, 0.f), 0.108f * H, 0.116f * H) * 0.74f;
    d = smin(d, neck, 0.035f * H);
    d = groove(d, q.x - 0.035f * H, GW, GD, GK);
  }
  // a cross seam a third of the way up the plates
  {
    float arc = len2(vec2(q.x + 0.42f * H, q.y)) - 0.66f * H;
    d = grooveAbove(d, arc, GW * 0.9f, GD, GK, q.z, 0.0f);
  }
  // bearing cap with a ring of beads on the index knuckle (thumb side)
  {
    const Bone& b = rF[1].ph[0];
    vec3 l = b.loc(q);
    vec3 lp = vec3(l.y - 0.80f * b.ra, l.x, l.z);
    float cap = sdRoundCylX(lp, 0.28f * b.ra, 0.56f * b.ra, 0.10f * b.ra);
    d = std::fmin(d, cap);
    d = smin(d, beadRing(lp, 0.24f * b.ra, 0.56f * b.ra, 0.16f * b.ra, 8), 0.03f * b.ra);
    d = smin(d, sdSphere(lp - vec3(0.30f * b.ra, 0.f, 0.f), 0.20f * b.ra), 0.04f * b.ra);  // domed pin head
  }
  // --- fingers
  for (int i = 1; i < 5; i++) {
    const Chain& c = rF[i];
    if (length(q - c.bc) - c.br > d + 0.2f) continue;
    d = std::fmin(d, robotFinger(q, c));
  }
  // --- thumb: a true cone of three segments with an oval hatch, beaded
  // collars, and a rounded cap that lies in front of the curled fingers
  {
    const Chain& c = rF[0];
    if (length(q - c.bc) - c.br < d + 0.2f) {
      float dt = robotShell(q, c.ph[0], false, true, true, 1.0f, 0.34f);
      dt = std::fmin(dt, robotCollar(q, c.ph[0], c.ph[1], 11));
      dt = std::fmin(dt, robotShell(q, c.ph[1], false, false, false, 1.0f, 0.30f));
      dt = std::fmin(dt, robotCollar(q, c.ph[1], c.ph[2], 10));
      dt = std::fmin(dt, robotShell(q, c.ph[2], true, false, false, 1.0f));
      d = smin(d, dt, 0.012f * H);
    }
  }
  return d;
}

inline void buildRobot() {
  const float H = P.Hr;
  struct F { vec3 mcp; float len, r0, r3, abd, f0, f1, f2; };
  // the curled fingers hang in an open C below the index, the middle lowest
  const F f[4] = {
      {{0.520f, 0.150f, 0.004f}, 0.42f, 0.056f, 0.046f, -16.f, 10.f, 8.f, 4.f},    // index, angled down
      {{0.540f, 0.050f, 0.008f}, 0.44f, 0.050f, 0.041f, 0.f, 52.f, 46.f, 26.f},    // middle
      {{0.515f, -0.048f, 0.000f}, 0.41f, 0.048f, 0.039f, 3.f, 58.f, 48.f, 28.f},   // ring
      {{0.470f, -0.136f, -0.012f}, 0.33f, 0.044f, 0.036f, 7.f, 64.f, 50.f, 30.f},  // little
  };
  for (int i = 0; i < 4; i++) {
    float L = f[i].len * H;
    std::vector<float> Ls = {0.45f * L, 0.29f * L, 0.26f * L};
    float r0 = f[i].r0 * H, r3 = f[i].r3 * H;
    std::vector<float> rs = {r0, mix(r0, r3, 0.45f), mix(r0, r3, 0.78f), r3};
    mat3 R0 = rotX(rad(i == 0 ? -3.f : (i == 3 ? 5.f : 0.f)));
    rF[i + 1] = chain(f[i].mcp * H, R0, f[i].abd, {f[i].f0, f[i].f1, f[i].f2}, Ls, rs);
  }
  // thumb: from the wrist pod, down and forward, its cap ahead of the fingertips
  rF[0] = chainPts({vec3(0.07f, 0.100f, -0.020f) * H, vec3(0.31f, 0.215f, -0.120f) * H,
                    vec3(0.47f, 0.225f, -0.215f) * H, vec3(0.585f, 0.205f, -0.290f) * H},
                   normalize(vec3(0.10f, 0.80f, 0.45f)), {0.090f * H, 0.068f * H, 0.056f * H, 0.044f * H});
  for (int i = 0; i < 5; i++) rF[i].bound(0.05f);
}

}  // namespace model
