// Signed-distance primitives and operators for the sculpted Creation Ring.
// Units are millimetres. Written against GLM so the code reads like GLSL.
#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdint>

using glm::vec2;
using glm::vec3;
using glm::mat3;
using glm::dot;
using glm::cross;
using glm::length;
using glm::normalize;
using glm::clamp;
using glm::mix;

static constexpr float PI = 3.14159265358979f;
inline float rad(float d) { return d * PI / 180.0f; }
inline float sat(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
inline float sstep(float a, float b, float x) { float t = sat((x - a) / (b - a)); return t * t * (3.f - 2.f * t); }
inline float sq(float x) { return x * x; }
inline float len2(vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }

// ---------------------------------------------------------------- operators
// polynomial smooth min/max; k is the blend width in mm
inline float smin(float a, float b, float k) {
  if (k <= 0.f) return std::fmin(a, b);
  float h = std::fmax(k - std::fabs(a - b), 0.f) / k;
  return std::fmin(a, b) - h * h * h * k * (1.f / 6.f);
}
inline float smax(float a, float b, float k) { return -smin(-a, -b, k); }
// smooth subtraction of b from a
inline float ssub(float a, float b, float k) { return smax(a, -b, k); }

// ---------------------------------------------------------------- rotations
inline mat3 rotX(float a) { float c = std::cos(a), s = std::sin(a); return mat3(1, 0, 0, 0, c, s, 0, -s, c); }
inline mat3 rotY(float a) { float c = std::cos(a), s = std::sin(a); return mat3(c, 0, -s, 0, 1, 0, s, 0, c); }
inline mat3 rotZ(float a) { float c = std::cos(a), s = std::sin(a); return mat3(c, s, 0, -s, c, 0, 0, 0, 1); }
// rotation about an arbitrary unit axis
inline mat3 rotAxis(vec3 u, float a) {
  float c = std::cos(a), s = std::sin(a), t = 1.f - c;
  return mat3(t * u.x * u.x + c, t * u.x * u.y + s * u.z, t * u.x * u.z - s * u.y,
              t * u.x * u.y - s * u.z, t * u.y * u.y + c, t * u.y * u.z + s * u.x,
              t * u.x * u.z + s * u.y, t * u.y * u.z - s * u.x, t * u.z * u.z + c);
}
// orthonormal frame from a primary axis x and a hint for z
inline mat3 frameXZ(vec3 x, vec3 zHint) {
  x = normalize(x);
  vec3 y = normalize(cross(zHint, x));
  vec3 z = cross(x, y);
  return mat3(x, y, z);
}

// A rigid placement: world = R * local + o. Columns of R are the local axes.
struct Frame {
  mat3 R = mat3(1.f);
  vec3 o = vec3(0.f);
  vec3 toLocal(vec3 p) const { return glm::transpose(R) * (p - o); }
  vec3 toWorld(vec3 q) const { return R * q + o; }
  vec3 dirWorld(vec3 d) const { return R * d; }
};

// ---------------------------------------------------------------- primitives
inline float sdSphere(vec3 p, float r) { return length(p) - r; }

// Inigo Quilez's ellipsoid bound (good near the surface)
inline float sdEllipsoid(vec3 p, vec3 r) {
  float k0 = length(p / r);
  float k1 = length(p / (r * r));
  if (k1 < 1e-9f) return -std::fmin(r.x, std::fmin(r.y, r.z));
  return k0 * (k0 - 1.f) / k1;
}

inline float sdCapsule(vec3 p, vec3 a, vec3 b, float r) {
  vec3 pa = p - a, ba = b - a;
  float h = sat(dot(pa, ba) / dot(ba, ba));
  return length(pa - ba * h) - r;
}

// exact round cone between spheres (a, r1) and (b, r2) -- IQ
inline float sdRoundCone(vec3 p, vec3 a, vec3 b, float r1, float r2) {
  vec3 ba = b - a;
  float l2 = dot(ba, ba);
  float rr = r1 - r2;
  float a2 = l2 - rr * rr;
  float il2 = 1.f / l2;
  vec3 pa = p - a;
  float y = dot(pa, ba);
  float z = y - l2;
  vec3 xv = pa * l2 - ba * y;
  float x2 = dot(xv, xv);
  float y2 = y * y * l2;
  float z2 = z * z * l2;
  float k = (rr > 0 ? 1.f : (rr < 0 ? -1.f : 0.f)) * rr * rr * x2;
  if ((z > 0 ? 1.f : (z < 0 ? -1.f : 0.f)) * a2 * z2 > k) return std::sqrt(x2 + z2) * il2 - r2;
  if ((y > 0 ? 1.f : (y < 0 ? -1.f : 0.f)) * a2 * y2 < k) return std::sqrt(x2 + y2) * il2 - r1;
  return (std::sqrt(x2 * a2 * il2) + y * rr) * il2 - r1;
}

// capped cylinder along local x from x=0 to x=h, radius r, edges rounded by e
inline float sdRoundCylX(vec3 q, float h, float r, float e) {
  vec2 d = vec2(len2(vec2(q.y, q.z)) - r + e, std::fabs(q.x - h * 0.5f) - h * 0.5f + e);
  return std::fmin(std::fmax(d.x, d.y), 0.f) + length(glm::max(d, vec2(0.f))) - e;
}

// tapered, rounded cylinder along local x: radius r0 at x=0 to r1 at x=h,
// flat-ish ends with edge rounding e (a "can" segment for the robot)
inline float sdTaperCanX(vec3 q, float h, float r0, float r1, float e) {
  float t = sat(q.x / h);
  float r = r0 + (r1 - r0) * t;
  float slope = (r1 - r0) / h;
  float rho = len2(vec2(q.y, q.z));
  // distance to the slanted side, corrected for the taper
  float ds = (rho - r + e) / std::sqrt(1.f + slope * slope);
  float dx = std::fabs(q.x - h * 0.5f) - h * 0.5f + e;
  vec2 d = vec2(ds, dx);
  return std::fmin(std::fmax(d.x, d.y), 0.f) + length(glm::max(d, vec2(0.f))) - e;
}

inline float sdTorusX(vec3 q, float R, float r) {  // ring around the local x axis
  vec2 t = vec2(len2(vec2(q.y, q.z)) - R, q.x);
  return length(t) - r;
}

inline float sdRoundBox(vec3 p, vec3 b, float r) {
  vec3 q = glm::abs(p) - b + r;
  return length(glm::max(q, vec3(0.f))) + std::fmin(std::fmax(q.x, std::fmax(q.y, q.z)), 0.f) - r;
}

// 2D: distance to an ellipse (approximate, accurate near the curve)
inline float sdEllipse2(vec2 q, vec2 r) {
  vec2 n = q / r;
  float k0 = len2(n);
  float k1 = len2(n / r);
  if (k1 < 1e-9f) return -std::fmin(r.x, r.y);
  return k0 * (k0 - 1.f) / k1;
}

// 2D superellipse-ish rounded section: half extents a (radial), b (axial), exponent n
inline float sdSuperEllipse2(vec2 q, vec2 r, float n) {
  vec2 u = glm::abs(q) / r;
  float m = std::pow(std::pow(u.x, n) + std::pow(u.y, n), 1.f / n);
  // gradient-normalised to keep a usable distance near the boundary
  float gx = std::pow(u.x, n - 1.f) / r.x, gy = std::pow(u.y, n - 1.f) / r.y;
  float pm = std::pow(m, n - 1.f);
  float g = std::sqrt(gx * gx + gy * gy) / std::fmax(pm, 1e-6f);
  return (m - 1.f) / std::fmax(g, 1e-6f);
}

// ---------------------------------------------------------------- noise
inline float hash11(float n) { float s = std::sin(n) * 43758.5453123f; return s - std::floor(s); }
inline float hash21(vec2 p) {
  float h = dot(p, vec2(127.1f, 311.7f));
  float s = std::sin(h) * 43758.5453123f;
  return s - std::floor(s);
}
inline vec2 hash22(vec2 p) {
  vec2 q = vec2(dot(p, vec2(127.1f, 311.7f)), dot(p, vec2(269.5f, 183.3f)));
  vec2 s = vec2(std::sin(q.x), std::sin(q.y)) * 43758.5453123f;
  return s - glm::floor(s);
}
inline float hash31(vec3 p) {
  float h = dot(p, vec3(127.1f, 311.7f, 74.7f));
  float s = std::sin(h) * 43758.5453123f;
  return s - std::floor(s);
}
// smooth value noise in 3D, range ~[-1, 1]
inline float vnoise3(vec3 p) {
  vec3 i = glm::floor(p), f = p - i;
  vec3 u = f * f * (3.f - 2.f * f);
  float n000 = hash31(i), n100 = hash31(i + vec3(1, 0, 0)), n010 = hash31(i + vec3(0, 1, 0)), n110 = hash31(i + vec3(1, 1, 0));
  float n001 = hash31(i + vec3(0, 0, 1)), n101 = hash31(i + vec3(1, 0, 1)), n011 = hash31(i + vec3(0, 1, 1)), n111 = hash31(i + vec3(1, 1, 1));
  float x00 = mix(n000, n100, u.x), x10 = mix(n010, n110, u.x), x01 = mix(n001, n101, u.x), x11 = mix(n011, n111, u.x);
  float y0 = mix(x00, x10, u.y), y1 = mix(x01, x11, u.y);
  return mix(y0, y1, u.z) * 2.f - 1.f;
}
inline float fbm3(vec3 p, int oct) {
  float a = 0.5f, s = 0.f, n = 0.f;
  for (int i = 0; i < oct; i++) { s += a * vnoise3(p); n += a; p = p * 2.03f + vec3(1.7f, 9.2f, 3.1f); a *= 0.5f; }
  return s / n;
}
