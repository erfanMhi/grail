// The human hand: a slender right hand in the Sistine pose -- index reaching,
// the other fingers falling loosely toward the palm, the thumb tucked under.
#pragma once
#include "common.h"

namespace model {

// One phalanx: a flattened round cone with a palmar pad, a bony knuckle on
// the back of the joint, and on the last segment the pulp and a nail plate
// that follows the curve of the fingertip.
inline float humanPhalanx(vec3 q, const Bone& b, int seg, int nseg) {
  vec3 l = b.loc(q);
  const float sq = 0.90f;                       // a little flatter dorso-palmar
  vec3 ls = vec3(l.x, l.y, l.z / sq);
  float d = sdRoundCone(ls, vec3(0.f), vec3(b.len, 0.f, 0.f), b.ra, b.rb) * sq;
  float rm = 0.5f * (b.ra + b.rb);
  float pad = sdEllipsoid(l - vec3(0.50f * b.len, 0.f, -0.30f * rm), vec3(0.32f * b.len, 0.78f * rm, 0.55f * rm));
  d = smin(d, pad, 0.30f * rm);
  if (seg > 0) {
    float kn = sdEllipsoid(l - vec3(0.02f * b.len, 0.f, 0.44f * b.ra), vec3(0.38f * b.ra, 0.70f * b.ra, 0.44f * b.ra));
    d = smin(d, kn, 0.25f * b.ra);
  }
  if (seg == nseg - 1) {
    float pulp = sdEllipsoid(l - vec3(0.62f * b.len, 0.f, -0.14f * b.rb), vec3(0.42f * b.len, 0.94f * b.rb, 0.82f * b.rb));
    d = smin(d, pulp, 0.25f * b.rb);
    // nail: the fingertip's own surface lifted a hair inside an oval outline
    float xc = 0.75f * b.len;
    float outline = sdEllipse2(vec2(l.x - xc, l.y), vec2(0.48f * b.len, 0.64f * b.rb));
    outline = std::fmax(outline, -(l.z - 0.25f * b.rb));
    float nail = smax(d - 0.022f, outline, 0.015f);
    d = std::fmin(d, nail);
  }
  return d;
}

inline float humanFinger(vec3 q, const Chain& c) {
  float d = 1e9f;
  for (int i = 0; i < c.n; i++) {
    float di = humanPhalanx(q, c.ph[i], i, c.n);
    d = (i == 0) ? di : smin(d, di, 0.30f * c.ph[i].ra);
  }
  return d;
}

inline float humanHandLocal(vec3 q) {
  const float H = P.Hh;
  vec3 u = q / H;
  // --- palm: four metacarpals fused into a softly arched slab
  float d = 1e9f;
  const vec3 cmc[4] = {{0.13f, 0.090f, 0.004f}, {0.13f, 0.030f, 0.010f}, {0.125f, -0.030f, 0.004f}, {0.115f, -0.080f, -0.008f}};
  for (int i = 0; i < 4; i++) {
    const Bone& b = hF[i + 1].ph[0];
    float dm = sdRoundCone(q, cmc[i] * H, b.a - b.R[0] * (0.025f * H), 0.048f * H, 0.047f * H);
    d = (i == 0) ? dm : smin(d, dm, 0.11f * H);
  }
  // the back of the hand is a low dome; the palm a thicker pad
  d = smin(d, sdEllipsoid(u - vec3(0.31f, 0.012f, 0.010f), vec3(0.25f, 0.175f, 0.056f)) * H, 0.05f * H);
  d = smin(d, sdEllipsoid(u - vec3(0.36f, 0.015f, -0.044f), vec3(0.19f, 0.158f, 0.048f)) * H, 0.06f * H);
  // thenar eminence (the thumb's muscle) and hypothenar along the outer edge
  vec3 th = rotZ(rad(-28.f)) * (u - vec3(0.205f, 0.110f, -0.060f));
  d = smin(d, sdEllipsoid(th, vec3(0.150f, 0.066f, 0.064f)) * H, 0.065f * H);
  d = smin(d, sdEllipsoid(u - vec3(0.30f, -0.124f, -0.040f), vec3(0.19f, 0.050f, 0.054f)) * H, 0.06f * H);
  // carpus: the wrist is narrower and rounder than the palm
  d = smin(d, sdEllipsoid(u - vec3(0.05f, 0.0f, -0.006f), vec3(0.12f, 0.142f, 0.084f)) * H, 0.08f * H);
  // knuckles: the metacarpal heads
  for (int i = 0; i < 4; i++) {
    const Bone& b = hF[i + 1].ph[0];
    float k = sdSphere(q - (b.a - b.R[0] * (0.02f * H) + b.R[2] * (0.010f * H)), 0.049f * H);
    d = smin(d, k, 0.035f * H);
  }
  // extensor tendons, only just showing through the skin
  for (int i = 0; i < 4; i++) {
    const Bone& b = hF[i + 1].ph[0];
    vec3 a0 = b.a - b.R[0] * (0.07f * H) + b.R[2] * (0.038f * H);
    vec3 a1 = vec3(0.15f, 0.025f - 0.013f * i, 0.050f) * H;
    d = smin(d, sdCapsule(q, a0, a1, 0.0070f * H), 0.035f * H);
  }
  // a dorsal vein
  {
    vec3 v0 = vec3(0.42f, 0.105f, 0.046f) * H, v1 = vec3(0.29f, 0.050f, 0.056f) * H, v2 = vec3(0.15f, 0.070f, 0.058f) * H;
    float dv = std::fmin(sdCapsule(q, v0, v1, 0.0075f * H), sdCapsule(q, v1, v2, 0.0075f * H));
    d = smin(d, dv, 0.025f * H);
  }
  // first web: the fleshy fold between thumb and index
  {
    const Bone& t1 = hF[0].ph[1];
    const Bone& i0 = hF[1].ph[0];
    float web = sdCapsule(q, t1.a + t1.R[0] * (0.25f * t1.len), i0.a + i0.R[0] * (0.08f * H) - i0.R[2] * (0.015f * H), 0.034f * H);
    d = smin(d, web, 0.05f * H);
  }
  // --- fingers
  for (int i = 1; i < 5; i++) {
    const Chain& c = hF[i];
    if (length(q - c.bc) - c.br > d + 0.2f) continue;
    d = smin(d, humanFinger(q, c), (i == 1 ? 0.055f : 0.05f) * H);
  }
  // --- thumb: the metacarpal sinks into the thenar mass
  {
    const Chain& c = hF[0];
    if (length(q - c.bc) - c.br < d + 0.2f) {
      float dt = sdRoundCone(q, c.ph[0].a, c.ph[0].b, c.ph[0].ra, c.ph[0].rb);
      dt = smin(dt, humanPhalanx(q, c.ph[1], 1, 3), 0.30f * c.ph[1].ra);
      dt = smin(dt, humanPhalanx(q, c.ph[2], 2, 3), 0.28f * c.ph[2].ra);
      d = smin(d, dt, 0.06f * H);
    }
  }
  return d;
}

inline void buildHuman() {
  const float H = P.Hh;
  struct F { vec3 mcp; float len, r0, r3, abd, f0, f1, f2; };
  const F f[4] = {
      {{0.530f, 0.160f, 0.000f}, 0.392f, 0.052f, 0.040f, -16.f, 2.f, 12.f, 8.f},    // index
      {{0.550f, 0.055f, 0.008f}, 0.430f, 0.054f, 0.041f, 0.f, 64.f, 40.f, 20.f},    // middle
      {{0.525f, -0.048f, 0.000f}, 0.405f, 0.051f, 0.039f, 2.f, 72.f, 44.f, 22.f},   // ring
      {{0.480f, -0.140f, -0.018f}, 0.322f, 0.045f, 0.034f, 6.f, 78.f, 46.f, 24.f}, // little
  };
  for (int i = 0; i < 4; i++) {
    float L = f[i].len * H;
    std::vector<float> Ls = {0.455f * L, 0.290f * L, 0.255f * L};
    float r0 = f[i].r0 * H, r3 = f[i].r3 * H;
    std::vector<float> rs = {r0, mix(r0, r3, 0.45f), mix(r0, r3, 0.78f), r3};
    mat3 R0 = rotX(rad(i == 0 ? -4.f : (i == 3 ? 6.f : 0.f)));   // transverse arch
    hF[i + 1] = chain(f[i].mcp * H, R0, f[i].abd, {f[i].f0, f[i].f1, f[i].f2}, Ls, rs);
  }
  // thumb by its joints: CMC at the wrist, forward and under the index,
  // nail facing out to the thumb side
  hF[0] = chainPts({vec3(0.080f, 0.100f, -0.035f) * H, vec3(0.285f, 0.200f, -0.095f) * H,
                    vec3(0.425f, 0.190f, -0.150f) * H, vec3(0.525f, 0.150f, -0.195f) * H},
                   normalize(vec3(0.10f, 0.80f, 0.45f)), {0.054f * H, 0.050f * H, 0.045f * H, 0.039f * H});
  for (int i = 0; i < 5; i++) hF[i].bound(0.05f);
}

}  // namespace model
