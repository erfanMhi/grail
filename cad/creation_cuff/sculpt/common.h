// Shared parameters, skeleton and globals for the sculpted Creation Ring.
//
// World frame (same as the OpenSCAD templates): the ring lies in the XY plane,
// Z is the finger axis, the opening faces -Y, the robot hand is on -X and the
// human hand on +X. The two index fingers almost touch in front of the opening.
//
// Hand-local frame (both hands are modelled as RIGHT hands; the robot is
// mirrored when placed): x runs from the wrist toward the fingers, y toward
// the thumb, z out of the back of the hand; the origin is the wrist joint.
// Proportions are written for a unit hand (wrist crease to middle fingertip
// = 1) and scaled by the hand length H.
#pragma once
#include "sdf.h"

#include <cstdlib>
#include <string>
#include <vector>

namespace model {

struct Params {
  float Ri = 8.65f;            // inner radius (US 7, 17.3 mm)
  // band section half-sizes: a = radial, b = axial
  float aBack = 1.15f, bBack = 1.58f;     // at the back of the ring
  float aArm = 1.50f, bArm = 1.72f;       // where the band swells into forearms
  // hands
  float Hh = 9.8f;             // human hand length (wrist to middle fingertip)
  float Hr = 10.9f;             // robot hand length
  float tipGap = 0.45f;        // between the two index fingertips
  float tipY = 12.5f;         // fingertips sit this far in front of the axis
  float tipZ = -1.35f;         // and this far below the band's mid-plane
  // hand orientation: heading toward the front, pitch down, roll of the back
  // of the hand toward the viewer (degrees)
  float hHead = 26.f, hPitch = 7.f, hRoll = 4.f;
  float rHead = 24.f, rPitch = 7.f, rRoll = 4.f;
  // wrist: flexion (hand bends palm-ward) and ulnar deviation (degrees)
  float hWristFlex = 14.f, hWristDev = 10.f;
  float rWristFlex = 10.f, rWristDev = 8.f;
  // surface
  float facetDepth = 0.17f;   // carved facets on the band
  float molten = 0.12f;       // low-frequency undulation
  int seed = 7;
};
inline Params P;

// Override parameters from the environment for quick studies, e.g.
//   RING_PARAMS="rRoll=-4,hRoll=-4,Hh=9.6" ./mesher ...
inline void applyOverrides() {
  const char* s = std::getenv("RING_PARAMS");
  if (!s) return;
  struct E { const char* n; float* v; };
  E tab[] = {{"Ri", &P.Ri}, {"aBack", &P.aBack}, {"bBack", &P.bBack}, {"aArm", &P.aArm}, {"bArm", &P.bArm},
             {"Hh", &P.Hh}, {"Hr", &P.Hr}, {"tipGap", &P.tipGap}, {"tipY", &P.tipY}, {"tipZ", &P.tipZ},
             {"hHead", &P.hHead}, {"hPitch", &P.hPitch}, {"hRoll", &P.hRoll},
             {"rHead", &P.rHead}, {"rPitch", &P.rPitch}, {"rRoll", &P.rRoll},
             {"hWristFlex", &P.hWristFlex}, {"hWristDev", &P.hWristDev},
             {"rWristFlex", &P.rWristFlex}, {"rWristDev", &P.rWristDev},
             {"facetDepth", &P.facetDepth}, {"molten", &P.molten}};
  std::string all(s);
  size_t pos = 0;
  while (pos < all.size()) {
    size_t comma = all.find(',', pos);
    std::string kv = all.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
    size_t eq = kv.find('=');
    if (eq != std::string::npos)
      for (auto& e : tab)
        if (kv.substr(0, eq) == e.n) *e.v = std::strtof(kv.c_str() + eq + 1, nullptr);
    if (comma == std::string::npos) break;
    pos = comma + 1;
  }
}

struct Bone {
  vec3 a, b;     // joint centres
  float ra, rb;  // radii at the joints
  mat3 R;        // columns: x along the bone, y lateral (thumb side), z dorsal
  float len;
  vec3 loc(vec3 q) const { return glm::transpose(R) * (q - a); }
};

struct Chain {
  Bone ph[4];
  int n = 0;
  vec3 bc;
  float br = 0;
  void bound(float pad) {
    vec3 lo(1e9f), hi(-1e9f);
    for (int i = 0; i < n; i++) {
      lo = glm::min(lo, glm::min(ph[i].a, ph[i].b) - vec3(ph[i].ra * 1.3f));
      hi = glm::max(hi, glm::max(ph[i].a, ph[i].b) + vec3(ph[i].ra * 1.3f));
    }
    bc = (lo + hi) * 0.5f;
    br = length(hi - lo) * 0.5f + pad;
  }
};

// Forward kinematics from a base joint: base frame (x distal, y thumb side,
// z dorsal), abduction about z, then flexion about y at each joint
// (+ bends toward the palm).
inline Chain chain(vec3 base, mat3 R0, float abd, std::vector<float> flex, std::vector<float> L, std::vector<float> r) {
  Chain c;
  c.n = (int)L.size();
  mat3 R = R0 * rotZ(rad(abd));
  vec3 p = base;
  for (int i = 0; i < c.n; i++) {
    R = R * rotY(rad(flex[i]));
    vec3 q = p + R[0] * L[i];
    c.ph[i] = Bone{p, q, r[i], r[i + 1], R, L[i]};
    p = q;
  }
  c.bound(0.f);
  return c;
}

// A chain through explicit joint positions; `dorsal` sets which way the back
// of each bone (the nail side) faces.
inline Chain chainPts(const std::vector<vec3>& J, vec3 dorsal, const std::vector<float>& r) {
  Chain c;
  c.n = (int)J.size() - 1;
  for (int i = 0; i < c.n; i++) {
    vec3 x = J[i + 1] - J[i];
    c.ph[i] = Bone{J[i], J[i + 1], r[i], r[i + 1], frameXZ(x, dorsal), length(x)};
  }
  c.bound(0.f);
  return c;
}

inline float hermite(float p0, float m0, float p1, float m1, float t) {
  float t2 = t * t, t3 = t2 * t;
  return (2 * t3 - 3 * t2 + 1) * p0 + (t3 - 2 * t2 + t) * m0 + (-2 * t3 + 3 * t2) * p1 + (t3 - t2) * m1;
}

// a thin groove cut into a surface: `sheet` is the signed distance to the
// groove's centre surface, w its half width, depth how far it sinks
inline float groove(float d, float sheet, float w, float depth, float k) {
  float g = std::fmax(std::fabs(sheet) - w, -(d + depth));
  return smax(d, -g, k);
}

// ---------------------------------------------------------------- globals
inline Frame FH, FR;                 // hand-local -> world (robot includes the mirror)
inline vec3 WH, WR, DH, DR;          // wrist centres and forearm directions (world)
inline Chain hF[5], rF[5];           // human / robot fingers: 0 thumb, 1 index .. 4 little
inline vec3 hBC, rBC;                // hand bounding spheres (world)
inline float hBR = 0, rBR = 0;
inline std::string part = "all";

}  // namespace model
