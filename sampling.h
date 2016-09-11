#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include "glm.h"

namespace framework {
  vec3 sample_hemisphere_uniform(vec2 uv) {
    float phi = uv.y * float(2.0 * M_PI);
    float cosTheta = 1.0f - uv.x;
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  vec3 sample_hemisphere_cos(vec2 uv) {
    float phi = uv.y * float(2.0 * M_PI);
    float cosTheta = sqrt(1.0f - uv.x);
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  vec3 sample_direction_cone(float u1, float u2, float cosThetaMax) {
    float cosTheta = (1.0f - u1) + u1 * cosThetaMax;
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    float phi = u2 * float(2 * M_PI);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  float sample_direction_cone_PDF(float cosThetaMax) {
    return 1.0f / (2.0f * float(M_PI) * (1.0f - cosThetaMax));
  }

  namespace detail {
    float radical_inverse(uint32_t b) {
      b = (b << 16u) | (b >> 16u);
      b = ((b & 0x55555555u) << 1u) | ((b & 0xAAAAAAAAu) >> 1u);
      b = ((b & 0x33333333u) << 2u) | ((b & 0xCCCCCCCCu) >> 2u);
      b = ((b & 0x0F0F0F0Fu) << 4u) | ((b & 0xF0F0F0F0u) >> 4u);
      b = ((b & 0x00FF00FFu) << 8u) | ((b & 0xFF00FF00u) >> 8u);
      return float(b) * 2.3283064365386963e-10f; // / 0x100000000
    }
  }
  
  // multiplication by inverse to allow constant floating after inlining
  inline vec2 hammersley_2d(uint32_t i, uint32_t N) {
    return vec2(i * (1.0 / N), detail::radical_inverse(i));
  }
};