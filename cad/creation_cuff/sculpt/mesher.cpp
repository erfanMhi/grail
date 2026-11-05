// Block-sparse naive surface nets for the sculpted Creation Ring.
//
//   mesher --h 0.025 --ply out.ply --stl out.stl [--part all|band|human|robot]
//          [--box x0 y0 z0 x1 y1 z1]
//
// The SDF is sampled on a regular grid of spacing h, but only inside 16^3
// blocks whose centre distance says the surface may pass through them. Every
// grid edge that changes sign emits a quad joining the four cells around it;
// every cell that carries a quad gets one vertex: the mean of its edge
// crossings, pulled onto the true surface by Newton steps. Normals come from
// the SDF gradient. The result is watertight and smooth at any resolution.
#include "model.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <omp.h>

struct Quad { uint64_t c[4]; };

static double now() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv) {
  float h = 0.03f;
  std::string ply, stl, part = "all";
  vec3 lo(-15.5f, -17.0f, -7.0f), hi(15.5f, 13.5f, 5.0f);
  float lip = 1.6f;  // safety factor on the distance bound for block culling
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--h") h = std::atof(argv[++i]);
    else if (a == "--ply") ply = argv[++i];
    else if (a == "--stl") stl = argv[++i];
    else if (a == "--part") part = argv[++i];
    else if (a == "--lip") lip = std::atof(argv[++i]);
    else if (a == "--box") { lo.x = std::atof(argv[++i]); lo.y = std::atof(argv[++i]); lo.z = std::atof(argv[++i]);
                             hi.x = std::atof(argv[++i]); hi.y = std::atof(argv[++i]); hi.z = std::atof(argv[++i]); }
    else { std::fprintf(stderr, "unknown arg %s\n", a.c_str()); return 2; }
  }
  model::init(part);
  const int B = 16;
  const int64_t Nx = (int64_t)std::ceil((hi.x - lo.x) / h), Ny = (int64_t)std::ceil((hi.y - lo.y) / h), Nz = (int64_t)std::ceil((hi.z - lo.z) / h);
  const int64_t Bx = (Nx + B - 1) / B, By = (Ny + B - 1) / B, Bz = (Nz + B - 1) / B;
  std::fprintf(stderr, "grid %lld x %lld x %lld (h=%.4f), blocks %lld\n", (long long)Nx, (long long)Ny, (long long)Nz, h, (long long)(Bx * By * Bz));
  auto P = [&](int64_t i, int64_t j, int64_t k) { return vec3(lo.x + i * h, lo.y + j * h, lo.z + k * h); };
  auto cellKey = [&](int64_t i, int64_t j, int64_t k) -> uint64_t { return (uint64_t)((i * Ny + j) * Nz + k); };

  double t0 = now();
  const float rb = 0.5f * B * h * std::sqrt(3.f);
  std::vector<std::vector<Quad>> tq(omp_get_max_threads());
  std::atomic<long long> active(0);
  const int64_t nb = Bx * By * Bz;
#pragma omp parallel
  {
    std::vector<float> s((B + 1) * (B + 1) * (B + 1));
    auto& out = tq[omp_get_thread_num()];
#pragma omp for schedule(dynamic, 8)
    for (int64_t b = 0; b < nb; b++) {
      int64_t bi = b / (By * Bz), bj = (b / Bz) % By, bk = b % Bz;
      int64_t i0 = bi * B, j0 = bj * B, k0 = bk * B;
      vec3 c = P(i0, j0, k0) + vec3(0.5f * B * h);
      float dc = model::sdf(c);
      if (std::fabs(dc) > rb * lip + 2.f * h) continue;
      active++;
      for (int a = 0; a <= B; a++)
        for (int bb = 0; bb <= B; bb++)
          for (int cc = 0; cc <= B; cc++) {
            int64_t i = i0 + a, j = j0 + bb, k = k0 + cc;
            s[(a * (B + 1) + bb) * (B + 1) + cc] = (i <= Nx && j <= Ny && k <= Nz) ? model::sdf(P(i, j, k)) : 1.f;
          }
      auto S = [&](int a, int bb, int cc) { return s[(a * (B + 1) + bb) * (B + 1) + cc]; };
      for (int a = 0; a < B; a++)
        for (int bb = 0; bb < B; bb++)
          for (int cc = 0; cc < B; cc++) {
            int64_t i = i0 + a, j = j0 + bb, k = k0 + cc;
            if (i >= Nx || j >= Ny || k >= Nz) continue;
            float d0 = S(a, bb, cc);
            bool in0 = d0 < 0.f;
            // the three edges leaving this grid vertex in +x, +y, +z
            for (int ax = 0; ax < 3; ax++) {
              float d1 = ax == 0 ? S(a + 1, bb, cc) : (ax == 1 ? S(a, bb + 1, cc) : S(a, bb, cc + 1));
              bool in1 = d1 < 0.f;
              if (in0 == in1) continue;
              int64_t v[3] = {i, j, k};
              int ab = (ax + 1) % 3, ac = (ax + 2) % 3;
              if (v[ab] < 1 || v[ac] < 1) continue;
              const int off[4][2] = {{-1, -1}, {0, -1}, {0, 0}, {-1, 0}};
              Quad q;
              for (int n = 0; n < 4; n++) {
                int64_t cidx[3] = {v[0], v[1], v[2]};
                cidx[ab] += off[n][0];
                cidx[ac] += off[n][1];
                q.c[n] = cellKey(cidx[0], cidx[1], cidx[2]);
              }
              if (!in0) { std::swap(q.c[1], q.c[3]); }  // surface faces -axis: reverse
              out.push_back(q);
            }
          }
    }
  }
  size_t nq = 0;
  for (auto& v : tq) nq += v.size();
  std::fprintf(stderr, "pass 1: %lld active blocks, %zu quads, %.1fs\n", (long long)active.load(), nq, now() - t0);

  // unique cells -> vertices
  std::vector<uint64_t> keys;
  keys.reserve(nq * 4 / 3);
  for (auto& v : tq) for (auto& q : v) for (int n = 0; n < 4; n++) keys.push_back(q.c[n]);
  std::sort(keys.begin(), keys.end());
  keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
  const size_t nv = keys.size();
  std::vector<vec3> pos(nv), nrm(nv);
  double t1 = now();
#pragma omp parallel for schedule(dynamic, 256)
  for (int64_t n = 0; n < (int64_t)nv; n++) {
    uint64_t key = keys[n];
    int64_t k = key % Nz, j = (key / Nz) % Ny, i = key / (Ny * Nz);
    float d[8];
    vec3 cp[8];
    for (int c = 0; c < 8; c++) {
      cp[c] = P(i + (c & 1), j + ((c >> 1) & 1), k + ((c >> 2) & 1));
      d[c] = model::sdf(cp[c]);
    }
    const int E[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    vec3 acc(0.f);
    int cnt = 0;
    for (auto& e : E) {
      float a = d[e[0]], b = d[e[1]];
      if ((a < 0.f) == (b < 0.f)) continue;
      float t = a / (a - b);
      acc += cp[e[0]] + (cp[e[1]] - cp[e[0]]) * t;
      cnt++;
    }
    vec3 x = cnt ? acc / float(cnt) : cp[0] + vec3(0.5f * h);
    vec3 cmin = cp[0] - vec3(0.5f * h), cmax = cp[7] + vec3(0.5f * h);
    const float eps = h * 0.1f;
    for (int it = 0; it < 3; it++) {
      float f0 = model::sdf(x);
      vec3 g = model::grad(x, eps);
      float g2 = dot(g, g);
      if (g2 < 1e-12f) break;
      x -= g * (f0 / g2);
      x = glm::clamp(x, cmin, cmax);
      if (std::fabs(f0) < h * 1e-3f) break;
    }
    pos[n] = x;
    vec3 g = model::grad(x, eps);
    float gl = length(g);
    nrm[n] = gl > 1e-12f ? g / gl : vec3(0, 0, 1);
  }
  std::fprintf(stderr, "pass 2: %zu vertices, %.1fs\n", nv, now() - t1);

  auto idx = [&](uint64_t key) -> uint32_t { return (uint32_t)(std::lower_bound(keys.begin(), keys.end(), key) - keys.begin()); };
  std::vector<uint32_t> tri;
  tri.reserve(nq * 6);
  for (auto& v : tq)
    for (auto& q : v) {
      uint32_t a = idx(q.c[0]), b = idx(q.c[1]), c = idx(q.c[2]), d = idx(q.c[3]);
      // split along the shorter diagonal
      float dac = glm::length(pos[a] - pos[c]), dbd = glm::length(pos[b] - pos[d]);
      if (dac <= dbd) { tri.insert(tri.end(), {a, b, c, a, c, d}); }
      else { tri.insert(tri.end(), {a, b, d, b, c, d}); }
    }
  // drop degenerate triangles (two corners in the same cell can't happen, but
  // Newton can collapse a corner onto another)
  std::vector<uint32_t> clean;
  clean.reserve(tri.size());
  for (size_t t = 0; t < tri.size(); t += 3) {
    uint32_t a = tri[t], b = tri[t + 1], c = tri[t + 2];
    if (a == b || b == c || a == c) continue;
    vec3 n = cross(pos[b] - pos[a], pos[c] - pos[a]);
    if (dot(n, n) < 1e-20f) continue;
    clean.insert(clean.end(), {a, b, c});
  }
  tri.swap(clean);
  const size_t nt = tri.size() / 3;
  std::fprintf(stderr, "triangles: %zu, total %.1fs\n", nt, now() - t0);

  if (!ply.empty()) {
    FILE* f = std::fopen(ply.c_str(), "wb");
    std::fprintf(f, "ply\nformat binary_little_endian 1.0\nelement vertex %zu\nproperty float x\nproperty float y\nproperty float z\n"
                    "property float nx\nproperty float ny\nproperty float nz\nelement face %zu\nproperty list uchar uint vertex_indices\nend_header\n", nv, nt);
    std::vector<float> vb(nv * 6);
    for (size_t n = 0; n < nv; n++) {
      vb[n * 6 + 0] = pos[n].x; vb[n * 6 + 1] = pos[n].y; vb[n * 6 + 2] = pos[n].z;
      vb[n * 6 + 3] = nrm[n].x; vb[n * 6 + 4] = nrm[n].y; vb[n * 6 + 5] = nrm[n].z;
    }
    std::fwrite(vb.data(), sizeof(float), vb.size(), f);
    std::vector<uint8_t> fb(nt * 13);
    for (size_t t = 0; t < nt; t++) {
      fb[t * 13] = 3;
      std::memcpy(&fb[t * 13 + 1], &tri[t * 3], 12);
    }
    std::fwrite(fb.data(), 1, fb.size(), f);
    std::fclose(f);
  }
  if (!stl.empty()) {
    FILE* f = std::fopen(stl.c_str(), "wb");
    char header[80] = {0};
    std::snprintf(header, 80, "Creation Ring, sculpted SDF, h=%.3f mm", h);
    std::fwrite(header, 1, 80, f);
    uint32_t n32 = (uint32_t)nt;
    std::fwrite(&n32, 4, 1, f);
    for (size_t t = 0; t < nt; t++) {
      vec3 a = pos[tri[t * 3]], b = pos[tri[t * 3 + 1]], c = pos[tri[t * 3 + 2]];
      vec3 n = normalize(cross(b - a, c - a));
      float buf[12] = {n.x, n.y, n.z, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z};
      std::fwrite(buf, 4, 12, f);
      uint16_t attr = 0;
      std::fwrite(&attr, 2, 1, f);
    }
    std::fclose(f);
  }
  return 0;
}
