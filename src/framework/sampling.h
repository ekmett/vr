#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include "framework/glm.h"

namespace framework {
  vec3 sample_direction_cone(float u1, float u2, float cosThetaMax) {
    float cosTheta = (1.0f - u1) + u1 * cosThetaMax;
    float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
    float phi = u2 * 2.0f * M_PI;
    return vec3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
  }

  float sample_direction_cone_PDF(float cosThetaMax) {
    return 1.0f / (2.0f * float(M_PI) * (1.0f - cosThetaMax));
  }
};