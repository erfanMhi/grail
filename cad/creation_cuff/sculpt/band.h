// The band: a swept, forged section that runs from the human wrist round the
// back of the finger to the robot wrist.
#pragma once
#include "common.h"

namespace model {

struct Sample {
  vec3 C, T, N, B;   // centre, tangent (increasing theta), section axes
  float a, b;        // half sizes along N and B
  float asym;        // 1 = flat comfort-fit inner wall, 0 = plain ellipse
  float s;           // arc length from the human end
  float tex;         // weight of the carved surface
  float human;       // 1 near the human wrist (forearm anatomy)
  float psi = 0.f;   // twist of the section about the tangent (radians)
};
inline std::vector<Sample> S;
inline float thH = 0, thR = 0;   // polar angles of the two band ends (deg)
inline float spanDeg = 0;

// 2D section: flat-ish inner wall (n < 0), domed outside
inline float section(float xn, float xb, float a, float b, float asym) {
  float e = xn < 0.f ? mix(2.0f, 3.6f, asym) : mix(2.0f, 2.35f, asym);
  float d = sdSuperEllipse2(vec2(xn, xb), vec2(a, b), e);
  return std::fmax(d, -std::fmin(a, b));
}

// carved facets: soft scoops on a jittered lattice in (arc length, perimeter),
// each one elongated, tilted and as deep as its own hash says
inline float facets(float s, float ang, float rho, float cs, int nAround, float seed) {
  float cv = 2.f * PI / nAround;             // cell in angle
  float u = s / cs, v = ang / cv;
  vec2 iu = vec2(std::floor(u), std::floor(v));
  float acc = 0.f;
  for (int j = -1; j <= 1; j++)
    for (int i = -1; i <= 1; i++) {
      vec2 cell = iu + vec2(float(i), float(j));
      float wrapped = std::fmod(cell.y + 1000.f * nAround, (float)nAround);
      vec2 key = vec2(cell.x, wrapped) + vec2(seed * 3.1f, seed * 0.7f);
      vec2 jit = hash22(key);
      vec2 c = cell + vec2(0.1f, 0.1f) + jit * 0.8f;
      float du = (u - c.x) * cs, dv = (v - c.y) * cv * rho;
      float h = hash21(key + vec2(5.3f, 1.7f));
      float rot = (hash21(key + vec2(8.8f, 3.3f)) - 0.5f) * 1.6f;
      float cr = std::cos(rot), sr = std::sin(rot);
      float eu = du * cr - dv * sr, ev = du * sr + dv * cr;
      float rad = cs * (0.55f + 0.45f * h);
      float r2 = (eu * eu * 0.45f + ev * ev) / (rad * rad);
      float k = std::fmax(1.f - r2, 0.f);
      float dent = k * k * (0.35f + 1.1f * hash21(key + vec2(2.1f, 9.4f)));
      acc = smax(acc, dent, 0.22f);
    }
  return acc;
}

// Catmull-Rom through the samples: position and its first two derivatives
// (per unit of sample index), so the sweep is smooth across samples.
inline void crEval(float u, vec3& C, vec3& dC, vec3& ddC) {
  const int N = (int)S.size() - 1;
  int j = std::min(std::max((int)std::floor(u), 0), N - 1);
  float t = u - j;
  const vec3& P0 = S[std::max(j - 1, 0)].C;
  const vec3& P1 = S[j].C;
  const vec3& P2 = S[j + 1].C;
  const vec3& P3 = S[std::min(j + 2, N)].C;
  vec3 c1 = 0.5f * (P2 - P0), c2 = 0.5f * (2.f * P0 - 5.f * P1 + 4.f * P2 - P3), c3 = 0.5f * (-P0 + 3.f * P1 - 3.f * P2 + P3);
  C = P1 + t * (c1 + t * (c2 + t * c3));
  dC = c1 + t * (2.f * c2 + 3.f * t * c3);
  ddC = 2.f * c2 + 6.f * t * c3;
}
inline float crScalar(float u, float Sample::*m) {
  const int N = (int)S.size() - 1;
  int j = std::min(std::max((int)std::floor(u), 0), N - 1);
  float t = u - j;
  float p0 = S[std::max(j - 1, 0)].*m, p1 = S[j].*m, p2 = S[j + 1].*m, p3 = S[std::min(j + 2, N)].*m;
  return p1 + 0.5f * t * ((p2 - p0) + t * ((2.f * p0 - 5.f * p1 + 4.f * p2 - p3) + t * (-p0 + 3.f * p1 - 3.f * p2 + p3)));
}

inline float band(vec3 p) {
  float r = len2(vec2(p.x, p.y));
  if (r < P.Ri - 2.5f) return (P.Ri - 0.2f) - r;
  float th = std::atan2(p.y, p.x) * 180.f / PI;
  float uu = std::fmod(th - thH + 720.f, 360.f);
  float gap = 360.f - spanDeg;
  if (uu > spanDeg + gap * 0.5f) uu -= 360.f;
  const int N = (int)S.size() - 1;
  int lo, hi;
  if (uu < 0) { lo = 0; hi = 16; }
  else if (uu > spanDeg) { lo = N - 16; hi = N; }
  else { int ie = (int)(uu / spanDeg * N); lo = ie - 12; hi = ie + 12; }
  lo = std::max(lo, 0);
  hi = std::min(hi, N);
  float best = 1e9f, u = 0.f;
  for (int j = lo; j < hi; j++) {
    vec3 ab = S[j + 1].C - S[j].C;
    vec3 ap = p - S[j].C;
    float t = sat(dot(ap, ab) / dot(ab, ab));
    vec3 dv = ap - ab * t;
    float d2 = dot(dv, dv);
    if (d2 < best) { best = d2; u = j + t; }
  }
  // refine the foot point on the smooth curve
  vec3 C, dC, ddC;
  for (int it = 0; it < 3; it++) {
    crEval(u, C, dC, ddC);
    vec3 w = C - p;
    float g = dot(w, dC), gp = dot(dC, dC) + dot(w, ddC);
    if (gp <= 1e-9f) break;
    u = clamp(u - g / gp, 0.f, (float)N);
  }
  crEval(u, C, dC, ddC);
  vec3 T = normalize(dC);
  vec3 er = normalize(vec3(C.x, C.y, 0.f));
  vec3 n0 = normalize(er - T * dot(er, T));
  vec3 b0 = cross(n0, T);
  float psi = crScalar(u, &Sample::psi);
  vec3 Nn = n0 * std::cos(psi) + b0 * std::sin(psi);
  vec3 Bb = -n0 * std::sin(psi) + b0 * std::cos(psi);
  float a = crScalar(u, &Sample::a), b = crScalar(u, &Sample::b);
  float asym = crScalar(u, &Sample::asym);
  float sl = crScalar(u, &Sample::s);
  float tex = crScalar(u, &Sample::tex);
  vec3 v = p - C;
  float xn = dot(v, Nn), xb = dot(v, Bb), xt = dot(v, T);
  float d2 = section(xn, xb, a, b, asym);
  // rounded caps past the ends (they sit inside the wrists)
  float beyond = 0.f;
  if (u <= 0.f) beyond = -xt;
  if (u >= (float)N) beyond = xt;
  float d = d2;
  if (beyond > 0.f) {
    // a rounded, egg-shaped end that the wrist's own volume swallows
    d = sdEllipsoid(vec3(xn, xb, beyond), vec3(a, b, 0.9f * std::fmin(a, b)));
  }
  if (tex > 0.f && d < 0.8f) {
    float ang = std::atan2(xb, xn);
    float outer = sstep(-0.70f, 0.10f, xn / a);        // keep the inside smooth
    float rho = 0.5f * (a + b);
    float big = facets(sl, ang, rho, 3.4f, 4, float(P.seed));
    float small = facets(sl * 1.07f + 3.3f, ang + 0.4f, rho, 1.25f, 9, float(P.seed) + 11.f);
    float m = fbm3(p * 0.30f + vec3(3.1f, 1.3f, float(P.seed)), 3);
    // a couple of long ridges winding along the band, as if it had been twisted
    float tw = std::sin(ang * 2.f - sl * 0.33f + 1.7f * fbm3(p * 0.3f + vec3(7.f), 2));
    float ridge = -std::pow(1.f - std::fabs(tw), 6.f);
    d += tex * outer * (P.facetDepth * (big + 0.15f * small) + P.molten * m + 0.06f * ridge);
    // the inside only undulates
    d += tex * (1.f - outer) * 0.03f * fbm3(p * 0.6f + vec3(9.f, 2.f, 4.f), 2);
  }
  return d;
}

inline void buildBand() {
  // the band ends a little inside each wrist
  // the band runs on past each wrist into the palm, thinning to a point
  // there, so the forearm dives into the hand instead of ending in a cap
  vec3 eH = WH + DH * (0.04f * P.Hh), eR = WR + DR * (0.05f * P.Hr);
  thH = std::atan2(eH.y, eH.x) * 180.f / PI;
  thR = std::atan2(eR.y, eR.x) * 180.f / PI;
  if (thR < thH) thR += 360.f;
  spanDeg = thR - thH;
  float thWH = std::atan2(WH.y, WH.x) * 180.f / PI, thWR = std::atan2(WR.y, WR.x) * 180.f / PI;
  if (thWH < thH - 180.f) thWH += 360.f;
  if (thWR < thH) thWR += 360.f;
  const float inH = thWH - thH, inR = thR - thWR;     // degrees of band inside each hand
  const int N = 420;
  S.assign(N + 1, Sample());
  const float blendH = 48.f, blendR = 44.f;     // degrees over which the forearms leave the circle
  const float RcBack = P.Ri + P.aBack;
  // polar derivatives that make the end tangents follow the forearms
  auto endSlopes = [&](vec3 e, vec3 D, float th, float& R, float& dR, float& Z, float& dZ) {
    vec3 er = vec3(std::cos(rad(th)), std::sin(rad(th)), 0.f), et = vec3(-std::sin(rad(th)), std::cos(rad(th)), 0.f);
    R = len2(vec2(e.x, e.y));
    Z = e.z;
    float dt = dot(D, et);
    dR = R * dot(D, er) / dt;   // per radian
    dZ = R * D.z / dt;
  };
  float RH, dRH, ZH, dZH, RR_, dRR, ZR, dZR;
  // the band runs from the human end with increasing theta, so the direction
  // "into the hand" points backwards along theta at that end
  endSlopes(eH, DH, thH, RH, dRH, ZH, dZH);
  endSlopes(eR, DR, thR, RR_, dRR, ZR, dZR);
  for (int i = 0; i <= N; i++) {
    float th = thH + spanDeg * i / N;
    float fromH = th - thH, fromR = thR - th;
    float R = RcBack, Z = 0.f;
    // radius grows a little where the band swells into the forearms
    float swellH = sstep(blendH + 40.f, 6.f, fromH), swellR = sstep(blendR + 40.f, 6.f, fromR);
    float a = mix(P.aBack, P.aArm, std::fmax(swellH, swellR));
    float b = mix(P.bBack, P.bArm, std::fmax(swellH, swellR));
    R = P.Ri + a;
    if (fromH < blendH) {
      float t = fromH / blendH;
      float R1 = P.Ri + mix(P.aBack, P.aArm, sstep(blendH + 40.f, 6.f, blendH));
      R = hermite(RH, dRH * rad(blendH), R1, 0.f, t);
      Z = hermite(ZH, dZH * rad(blendH), 0.f, 0.f, t);
    }
    if (fromR < blendR) {
      float t = fromR / blendR;
      float R1 = P.Ri + mix(P.aBack, P.aArm, sstep(blendR + 40.f, 6.f, blendR));
      R = hermite(RR_, -dRR * rad(blendR), R1, 0.f, t);
      Z = hermite(ZR, -dZR * rad(blendR), 0.f, 0.f, t);
    }
    // gentle low-frequency wander so the band looks forged, not turned
    float wob = std::sin(rad(th * 2.f + 40.f)) * 0.06f + std::sin(rad(th * 3.f + 10.f)) * 0.04f;
    Z += wob * sstep(0.f, 30.f, std::fmin(fromH, fromR));
    S[i].C = vec3(R * std::cos(rad(th)), R * std::sin(rad(th)), Z);
    // section tapers to the wrists, then thins away inside the hands
    float fwH = fromH - inH, fwR = fromR - inR;     // degrees from each wrist (negative = inside the hand)
    float wH = sstep(18.f, 0.f, fwH), wR = sstep(16.f, 0.f, fwR);
    float aw = 0.128f * P.Hh, bw = 0.075f * P.Hh;   // human wrist half sizes
    float awr = 0.112f * P.Hr, bwr = 0.082f * P.Hr;
    a = mix(a, aw, wH);
    b = mix(b, bw, wH);
    a = mix(a, awr, wR);
    b = mix(b, bwr, wR);
    float inside = std::fmax(sstep(0.f, -inH, fwH), sstep(0.f, -inR, fwR));
    a *= 1.f - 0.55f * inside;
    b *= 1.f - 0.55f * inside;
    // waviness of the thickness
    a *= 1.f + 0.045f * std::sin(rad(th * 5.f + 17.f)) * (1.f - std::fmax(wH, wR));
    b *= 1.f + 0.040f * std::sin(rad(th * 4.f + 61.f)) * (1.f - std::fmax(wH, wR));
    S[i].a = a;
    S[i].b = b;
    S[i].asym = 1.f - std::fmax(sstep(40.f, 4.f, fwH), sstep(40.f, 4.f, fwR));
    S[i].tex = sstep(16.f, 44.f, fwH) * sstep(12.f, 42.f, fwR);
    S[i].human = sstep(50.f, 0.f, fwH);
  }
  // tangents, frames, arc length, and the twist into each wrist
  float sacc = 0.f;
  for (int i = 0; i <= N; i++) {
    vec3 T = normalize(S[std::min(i + 1, N)].C - S[std::max(i - 1, 0)].C);
    float th = thH + spanDeg * i / N;
    vec3 er = vec3(std::cos(rad(th)), std::sin(rad(th)), 0.f);
    vec3 n0 = normalize(er - T * dot(er, T));
    vec3 b0 = cross(n0, T);
    S[i].T = T;
    S[i].N = n0;
    S[i].B = b0;
    if (i > 0) sacc += length(S[i].C - S[i - 1].C);
    S[i].s = sacc;
  }
  auto twist = [&](int i, vec3 Zh, float w) {
    vec3 T = S[i].T;
    vec3 z = normalize(Zh - T * dot(Zh, T));
    S[i].psi += std::atan2(-dot(z, S[i].N), dot(z, S[i].B)) * w;
  };
  for (int i = 0; i <= N; i++) {
    float th = thH + spanDeg * i / N;
    float wH = sstep(blendH, 0.f, th - thH), wR = sstep(blendR, 0.f, thR - th);
    if (wH > 0.f) twist(i, FH.R[2], wH);
    if (wR > 0.f) twist(i, FR.R[2], wR);
  }
}

}  // namespace model
