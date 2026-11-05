// The robot hand: the human hand's skeleton rebuilt as polished machine parts.
// Language: tapered shells with rounded rims, ball joints showing in the gaps,
// beaded collars at the finger joints, a few fine panel lines, tip caps with
// an oval nail panel. The back of the hand is one smooth domed shell.
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

inline float robotShell(vec3 q, const Bone& b, bool tip, bool first, bool panel) {
  vec3 l = b.loc(q);
  const float sq = 0.93f;
  float x0 = (first ? 0.60f : 0.36f) * b.ra;
  float x1 = tip ? b.len : b.len - 0.36f * b.rb;
  float r0 = b.ra * 0.98f, r1 = b.rb;
  vec3 ls = vec3(l.x - x0, l.y, l.z / sq);
  float d;
  if (tip) {
    float L = std::fmax(x1 - x0 - 0.85f * r1, 0.1f * b.len);
    d = sdRoundCone(ls, vec3(0.f), vec3(L, 0.f, 0.f), r0, r1 * 0.88f) * sq;
    d = smax(d, -ls.x, 0.12f * r0);                       // flat rim at the joint
    float xc = L * 0.72f + 0.25f * r1;
    float outline = sdEllipse2(vec2(ls.x - xc, l.y), vec2(0.40f * L + 0.38f * r1, 0.56f * r1));
    d = grooveAbove(d, outline, 0.026f * r1, 0.07f * r1, 0.02f * r1, l.z, 0.15f * r1);
  } else {
    d = sdTaperCanX(ls, x1 - x0, r0, r1 + 0.25f * (r0 - r1), 0.14f * r0) * sq;
    float xs = x0 + (x1 - x0) * 0.36f;
    d = groove(d, l.x - xs, 0.022f * b.ra, 0.07f * b.ra, 0.016f * b.ra);
    if (panel) {   // an oval hatch on the back of the big thumb segment
      float xm = x0 + (x1 - x0) * 0.62f;
      float outline = sdEllipse2(vec2(l.x - xm, l.y), vec2(0.30f * (x1 - x0), 0.50f * b.ra));
      d = grooveAbove(d, outline, 0.022f * b.ra, 0.07f * b.ra, 0.016f * b.ra, l.z, 0.2f * b.ra);
    }
  }
  return d;
}

// beaded collar between two segments: a washer with a ring of beads, set on
// the bisector of the two bones so it reads at any flexion
inline float robotCollar(vec3 q, const Bone& prev, const Bone& next, int nb) {
  vec3 ax = normalize(prev.R[0] + next.R[0]);
  mat3 R = frameXZ(ax, prev.R[2] + next.R[2]);
  vec3 l = glm::transpose(R) * (q - next.a);
  float r = next.ra;
  const float sq = 0.93f;
  vec3 ls = vec3(l.x, l.y, l.z / sq);
  float core = sdSphere(l, 0.76f * r);
  float washer = sdRoundCylX(ls + vec3(0.13f * r, 0.f, 0.f), 0.26f * r, 0.90f * r, 0.07f * r) * sq;
  float d = std::fmin(core, washer);
  float bead = beadRing(ls, 0.f, 0.92f * r, 0.105f * r, nb) * sq;
  return smin(d, bead, 0.02f * r);
}

inline float robotFinger(vec3 q, const Chain& c) {
  float d = sdSphere(c.ph[0].loc(q), c.ph[0].ra * 0.90f);     // knuckle ball
  for (int i = 0; i < c.n; i++) {
    d = std::fmin(d, robotShell(q, c.ph[i], i == c.n - 1, i == 0, false));
    if (i > 0) d = std::fmin(d, robotCollar(q, c.ph[i - 1], c.ph[i], 12));
  }
  return d;
}

inline float robotHandLocal(vec3 q) {
  const float H = P.Hr;
  vec3 u = q / H;
  float d = 1e9f;
  // --- the back of the hand: metacarpal slab under one domed shell
  const vec3 cmc[4] = {{0.10f, 0.058f, 0.008f}, {0.10f, 0.020f, 0.014f}, {0.095f, -0.020f, 0.008f}, {0.09f, -0.055f, -0.002f}};
  for (int i = 0; i < 4; i++) {
    const Bone& b = rF[i + 1].ph[0];
    float dm = sdRoundCone(q, cmc[i] * H, b.a - b.R[0] * (0.03f * H), 0.050f * H, 0.055f * H);
    d = (i == 0) ? dm : smin(d, dm, 0.05f * H);
  }
  d = smin(d, sdEllipsoid(u - vec3(0.36f, 0.0f, 0.012f), vec3(0.20f, 0.150f, 0.056f)) * H, 0.03f * H);
  d = smin(d, sdEllipsoid(u - vec3(0.36f, 0.0f, -0.035f), vec3(0.19f, 0.145f, 0.055f)) * H, 0.03f * H);
  // housing for the thumb's root
  vec3 th = rotZ(rad(-28.f)) * (u - vec3(0.20f, 0.105f, -0.060f));
  d = smin(d, sdEllipsoid(th, vec3(0.13f, 0.072f, 0.066f)) * H, 0.025f * H);
  // short wrist neck that the band runs into
  {
    vec3 ln = vec3(q.x, q.y, q.z / 0.74f);
    float neck = sdRoundCone(ln, vec3(-0.05f * H, 0.f, 0.f), vec3(0.09f * H, 0.f, 0.f), 0.108f * H, 0.118f * H) * 0.74f;
    d = smin(d, neck, 0.04f * H);
  }
  // panel lines: the knuckle guard's arc, two seams between the plates, and
  // the wrist seam where the hand meets the band
  {
    float arc = len2(vec2(q.x + 0.30f * H, q.y)) - 0.735f * H;
    d = grooveAbove(d, arc, 0.0045f * H, 0.012f * H, 0.003f * H, q.z, -0.01f * H);
    vec2 a0 = vec2(0.12f, 0.040f) * H, a1 = vec2(0.425f, 0.100f) * H;
    vec2 b0 = vec2(0.12f, -0.035f) * H, b1 = vec2(0.42f, -0.092f) * H;
    auto seg2 = [](vec2 p, vec2 a, vec2 b) { vec2 pa = p - a, ba = b - a; float h = sat(dot(pa, ba) / dot(ba, ba)); return len2(pa - ba * h); };
    vec2 pxy = vec2(q.x, q.y);
    d = grooveAbove(d, seg2(pxy, a0, a1), 0.0040f * H, 0.012f * H, 0.003f * H, q.z, 0.0f);
    d = grooveAbove(d, seg2(pxy, b0, b1), 0.0040f * H, 0.012f * H, 0.003f * H, q.z, 0.0f);
    d = groove(d, q.x - 0.035f * H, 0.0050f * H, 0.014f * H, 0.003f * H);
  }
  // bearing cap with a bead ring on the index knuckle (the thumb side)
  {
    const Bone& b = rF[1].ph[0];
    vec3 l = b.loc(q);
    vec3 lp = vec3(l.y - 0.86f * b.ra, l.x, l.z);   // axis along the pin (local y)
    float cap = sdRoundCylX(lp, 0.26f * b.ra, 0.52f * b.ra, 0.10f * b.ra);
    d = std::fmin(d, cap);
    d = smin(d, beadRing(lp, 0.20f * b.ra, 0.52f * b.ra, 0.085f * b.ra, 10), 0.02f * b.ra);
  }
  // --- fingers
  for (int i = 1; i < 5; i++) {
    const Chain& c = rF[i];
    if (length(q - c.bc) - c.br > d + 0.2f) continue;
    d = std::fmin(d, robotFinger(q, c));
  }
  // --- thumb: a big tapered can with an oval hatch, then two segments
  {
    const Chain& c = rF[0];
    if (length(q - c.bc) - c.br < d + 0.2f) {
      float dt = robotShell(q, c.ph[0], false, true, true);
      dt = std::fmin(dt, robotCollar(q, c.ph[0], c.ph[1], 14));
      dt = std::fmin(dt, robotShell(q, c.ph[1], false, false, false));
      dt = std::fmin(dt, robotCollar(q, c.ph[1], c.ph[2], 12));
      dt = std::fmin(dt, robotShell(q, c.ph[2], true, false, false));
      d = smin(d, dt, 0.012f * H);
    }
  }
  return d;
}

inline void buildRobot() {
  const float H = P.Hr;
  struct F { vec3 mcp; float len, r0, r3, abd, f0, f1, f2; };
  const F f[4] = {
      {{0.520f, 0.150f, 0.004f}, 0.42f, 0.056f, 0.045f, -16.f, 0.f, 10.f, 6.f},     // index
      {{0.540f, 0.050f, 0.010f}, 0.44f, 0.058f, 0.046f, 0.f, 72.f, 44.f, 24.f},    // middle
      {{0.515f, -0.050f, 0.002f}, 0.41f, 0.055f, 0.044f, 2.f, 78.f, 50.f, 26.f},   // ring
      {{0.470f, -0.140f, -0.012f}, 0.33f, 0.048f, 0.039f, 5.f, 84.f, 54.f, 28.f},  // little
  };
  for (int i = 0; i < 4; i++) {
    float L = f[i].len * H;
    std::vector<float> Ls = {0.45f * L, 0.29f * L, 0.26f * L};
    float r0 = f[i].r0 * H, r3 = f[i].r3 * H;
    std::vector<float> rs = {r0, mix(r0, r3, 0.45f), mix(r0, r3, 0.78f), r3};
    mat3 R0 = rotX(rad(i == 0 ? -3.f : (i == 3 ? 5.f : 0.f)));
    rF[i + 1] = chain(f[i].mcp * H, R0, f[i].abd, {f[i].f0, f[i].f1, f[i].f2}, Ls, rs);
  }
  rF[0] = chainPts({vec3(0.08f, 0.100f, -0.020f) * H, vec3(0.32f, 0.220f, -0.120f) * H,
                    vec3(0.47f, 0.220f, -0.220f) * H, vec3(0.58f, 0.190f, -0.300f) * H},
                   normalize(vec3(0.10f, 0.80f, 0.45f)), {0.085f * H, 0.070f * H, 0.062f * H, 0.052f * H});
  for (int i = 0; i < 5; i++) rF[i].bound(0.05f);
}

}  // namespace model
