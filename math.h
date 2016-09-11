#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <type_traits>
#include "math_constexpr.h"
#include "glm.h"

namespace framework {


  // the simple case of the logistic map
  // https://en.wikipedia.org/wiki/Logistic_function
  inline float sigmoid(float x) {
    return 1 / (1 + exp(-x));
  }

  static inline vec3 perpendicular(const vec3 & v) {
    vec3 a = abs(v);
    float m = std::min(std::min(a.x, a.y), a.z);
    return normalize(
        (m == a.x) ? cross(v, vec3(1, 0, 0))
      : (m == a.y) ? cross(v, vec3(0, 1, 0))
                   : cross(v, vec3(0, 0, 1))
    );
  }

  constexpr inline float operator "" _degrees(long double d) noexcept {
    return float(d * M_PI / 180.f);
  }

  // input: azimuth & elevation
  inline vec3 spherical_to_cartesian(vec2 s) {
    auto cy = std::cos(s.y);
    return vec3{
      std::cos(s.x) * cy,
      std::sin(s.y),
      std::sin(s.x) * cy
    };
  }

  inline vec2 cartesian_to_spherical(vec3 c) {
    float azimuth = std::atan2(c.z, c.x);
    if (azimuth < 0.0f)
      azimuth = 2.0f * float(M_PI) + azimuth;
    return vec2(azimuth, std::asin(c.y));
  }

  template <typename T> T radians_to_degrees(T radians) {
#pragma warning( push )
#pragma warning( disable : 4305 )
    return radians * T(180.0 / M_PI);
#pragma warning( pop )
  }

  template <typename T> T degrees_to_radians(T degrees) {
#pragma warning( push )
#pragma warning( disable : 4305 )
    return degrees * T(M_PI / 180.0);
#pragma warning( pop )
  }

  using std::pow;

  template <int N> float pow(float x) {    
    return N > 0  ? exp(x * log(N))
         : N == 0 ? 1.0f
                  : 1 / exp(x * log(N));
  }


}