// The sculpted Creation Ring: one signed-distance field in millimetres.
// See common.h for the frames; band.h, human.h and robot.h for the parts.
#pragma once
#include "band.h"
#include "human.h"
#include "robot.h"

namespace model {

// world frame of a hand from heading / pitch / roll
inline mat3 handBasis(bool robot, float head, float pitch, float roll) {
  vec3 X0 = robot ? vec3(1, 0, 0) : vec3(-1, 0, 0);
  mat3 F0 = mat3(X0, cross(vec3(0, 0, 1), X0), vec3(0, 0, 1));
  mat3 F = rotZ(rad(robot ? -head : head)) * F0 * rotY(rad(pitch)) * rotX(rad(robot ? roll : -roll));
  return F;
}

// The real fingertip: march out along the last bone until we leave the surface.
template <class F>
inline vec3 tipAlong(const Chain& c, F sdfLocal) {
  const Bone& b = c.ph[c.n - 1];
  float t = 0.f;
  for (int i = 0; i < 600 && sdfLocal(b.b + b.R[0] * t) < 0.f; i++) t += 0.002f;
  return b.b + b.R[0] * t;
}

inline void place() {
  const mat3 M = mat3(1, 0, 0, 0, -1, 0, 0, 0, 1);
  // human
  FH.R = handBasis(false, P.hHead, P.hPitch, P.hRoll);
  vec3 tipH = vec3(P.tipGap * 0.5f, -P.tipY, P.tipZ + 0.15f);   // the human tip a touch higher, as in the photo
  FH.o = tipH - FH.R * tipAlong(hF[1], humanHandLocal);
  // robot: local -> world is R * M
  mat3 RR = handBasis(true, P.rHead, P.rPitch, P.rRoll);
  FR.R = RR * M;
  vec3 tipR = vec3(-P.tipGap * 0.5f, -P.tipY, P.tipZ);
  FR.o = tipR - FR.R * tipAlong(rF[1], robotHandLocal);
  WH = FH.o;
  WR = FR.o;
  // forearm directions (pointing from the band into the hand)
  auto fore = [](float flex, float dev) { return normalize(vec3(std::cos(rad(flex)) * std::cos(rad(dev)), std::sin(rad(dev)), std::sin(rad(flex)))); };
  DH = FH.R * fore(P.hWristFlex, P.hWristDev);
  DR = FR.R * fore(P.rWristFlex, P.rWristDev);
}

inline vec3 handBoundCenter(const Chain* F, int n, const Frame& fr, float H, float& rad_) {
  vec3 lo(1e9f), hi(-1e9f);
  for (int i = 0; i < n; i++) {
    lo = glm::min(lo, F[i].bc - vec3(F[i].br));
    hi = glm::max(hi, F[i].bc + vec3(F[i].br));
  }
  lo = glm::min(lo, vec3(-0.25f, -0.25f, -0.25f) * H);
  hi = glm::max(hi, vec3(0.25f, 0.25f, 0.25f) * H);
  rad_ = length(hi - lo) * 0.5f + 0.3f;
  return fr.toWorld((lo + hi) * 0.5f);
}

// US ring size -> inside diameter in mm (the standard chart: 7 = 17.32, 11 = 20.57)
inline float usInsideDiameter(float size) { return 11.63f + 0.8128f * size; }

// Design -> finished scale. The bore is cut at Ri - 0.02 and the rounding of
// that cut leaves the narrowest point 0.008 mm further out (measured on the
// mesh: 8.638 mm at Ri = 8.65), so the finished bore is exactly the US size.
inline float sizeScale() { return usInsideDiameter(P.usSize) / (2.f * (P.Ri - 0.012f)); }

inline void init(const std::string& which) {
  part = which;
  applyOverrides();
  buildHuman();
  buildRobot();
  place();
  buildBand();
  hBC = handBoundCenter(hF, 5, FH, P.Hh, hBR);
  rBC = handBoundCenter(rF, 5, FR, P.Hr, rBR);
  ringScale = sizeScale();
}

// the ring as designed, in design mm
inline float designSdf(vec3 p) {
  if (part == "humanL") return humanHandLocal(p);
  if (part == "robotL") return robotHandLocal(p);
  float d = 1e9f;
  if (part == "all" || part == "band") d = band(p);
  if (part == "all" || part == "human") {
    float bd = length(p - hBC) - hBR;
    float dh = bd > 0.5f ? bd : humanHandLocal(FH.toLocal(p));
    // a broad blend right at the wrist so the carpus melts into the forearm,
    // a tighter one elsewhere so the fingers never web onto the band
    float kw = mix(0.22f, 0.10f, sstep(3.2f, 1.2f, length(p - WH)));
    d = (part == "all") ? smin(d, dh, kw) : dh;
  }
  if (part == "all" || part == "robot") {
    float bd = length(p - rBC) - rBR;
    float dr = bd > 0.5f ? bd : robotHandLocal(FR.toLocal(p));
    d = (part == "all") ? smin(d, dr, 0.12f) : dr;
  }
  if (part == "all") d = smax(d, (P.Ri - 0.02f) - len2(vec2(p.x, p.y)), 0.15f);  // keep the bore clear
  return d;
}

// the finished ring at the wearer's size, in mm (an exact uniform scaling)
inline float sdf(vec3 p) { return ringScale * designSdf(p / ringScale); }

inline vec3 grad(vec3 p, float e) {
  const vec3 k0(1, -1, -1), k1(-1, -1, 1), k2(-1, 1, -1), k3(1, 1, 1);
  vec3 g = k0 * sdf(p + k0 * e) + k1 * sdf(p + k1 * e) + k2 * sdf(p + k2 * e) + k3 * sdf(p + k3 * e);
  return g / (4.f * e);
}

}  // namespace model
