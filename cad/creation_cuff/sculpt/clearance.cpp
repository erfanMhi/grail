// Bore clearance: how close do the hands come to the wearer's finger?
// Samples the hand SDFs on cylinders r = Ri + c (design mm) and reports the
// worst c that still intersects a hand (0 = touching the skin), converted to
// finished mm at the ring's US size.
#include "model.h"
#include <cstdio>
using namespace model;
int main() {
  init("all");
  std::printf("US %.2f: bore %.2f mm\n", P.usSize, usInsideDiameter(P.usSize));
  for (int hand = 0; hand < 2; hand++) {
    float worst = 1e9f;
    vec3 at(0.f);
    for (float c = -3.0f; c <= 1.5f && worst > 1e8f; c += 0.02f) {
      float R = P.Ri + c;
      for (float th = -180.f; th <= 0.f; th += 0.25f)
        for (float z = -7.f; z <= 4.f; z += 0.04f) {
          vec3 p(R * std::cos(rad(th)), R * std::sin(rad(th)), z);
          float d = hand == 0 ? humanHandLocal(FH.toLocal(p)) : robotHandLocal(FR.toLocal(p));
          if (d < 0.f && worst > 1e8f) { worst = c; at = p; }
        }
    }
    at *= ringScale;
    std::printf("%s: deepest point %.2f mm %s the bore surface (at %.2f %.2f %.2f)\n", hand == 0 ? "human" : "robot",
                std::fabs(worst) * ringScale, worst < 0 ? "INSIDE" : "outside", at.x, at.y, at.z);
  }
}
