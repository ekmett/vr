#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <intrin.h>
#include <type_traits>
#include "glm.h"

namespace framework {

  // sample uniformly from a sphere using [Archimedes' Hat-Box Theorem](http://mathworld.wolfram.com/ArchimedesHat-BoxTheorem.html).
  // input in (0,0) - (1,1)
  inline vec3 sample_sphere_uniform(vec2 uv) {
    float phi = uv.y * float(2 * M_PI);
    float cosTheta = 1 - 2 * uv.x;
    float sinTheta = sqrt(1 - cosTheta*cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  // sample uniformly from a hemisphere using [Archimedes' Hat-Box Theorem](http://mathworld.wolfram.com/ArchimedesHat-BoxTheorem.html)
  // input in (0,0) - (1,1)
  inline vec3 sample_hemisphere_uniform(vec2 uv) {
    float phi = uv.y * float(2 * M_PI);
    float cosTheta = 1.0f - uv.x;
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  // Another hat box
  // input in (0,0) - (1,1)
  inline vec3 sample_hemisphere_cos(vec2 uv, float cosThetaMax) {
    float phi = uv.y * float(2 * M_PI);
    float cosTheta = sqrt(1.0f - uv.x);
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  // Another hat box
  // input in (0,0) - (1,1)
  inline vec3 sample_direction_cone(vec2 uv, float cosThetaMax) {
    float phi = uv.y * float(2 * M_PI);
    float cosTheta = (1.0f - uv.x) + uv.x * cosThetaMax;
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  // input in (0,0) - (1,1)
  inline float constexpr sample_direction_cone_PDF(float cosThetaMax) {
    return 1.0f / (float(2 * M_PI) * (1.0f - cosThetaMax));
  }


  //--------------------------------------------------------------------------
  // Halton Low Discrepancy Sequence
  //--------------------------------------------------------------------------

  // may be useful if I add faure or halton
  namespace detail {
    // compute the base b expansion of k
    inline vector<int> b_ary(int k, int b) {
      vector<int> result(0);
      while (k >= b) {
        result.push_back(k % b);
        k = k / b;
      }
      return result;
    }
  }

};
