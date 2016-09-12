#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <type_traits>
#include "math_constexpr.h"
#include "glm.h"

namespace framework {
  static constexpr float pi = float(M_PI);
  static constexpr float pi_2 = float(M_PI_2);
  static constexpr float pi_4 = float(M_PI_4);

  static constexpr float tau = float(2 * M_PI);

  using math_constexpr::square;
  using math_constexpr::cube;

  // the simple case of the logistic map
  // https://en.wikipedia.org/wiki/Logistic_function
  static inline float sigmoid(float x) noexcept {
    return 1 / (1 + exp(-x));
  }

  static constexpr inline float rcp(float x) noexcept {
    return 1.f / x;
  }


  static inline vec3 perpendicular(const vec3 & v) noexcept {
    vec3 a = abs(v);
    float m = std::min(std::min(a.x, a.y), a.z);
    return normalize(
        (m == a.x) ? cross(v, vec3(1, 0, 0))
      : (m == a.y) ? cross(v, vec3(0, 1, 0))
                   : cross(v, vec3(0, 0, 1))
    );
  }

  // construct an arbitrary orthogonal coordinate frame with Z oriented towards N
  static inline mat3 frame(vec3 N) {
    vec3 X1 = vec3(0.0f, N.z, -N.y);
    vec3 X2 = vec3(-N.z, 0.0f, N.x);
    vec3 X = normalize(dot(X1, X1) <= dot(X2, X2) ? X2 : X1);
    return mat3(X, normalize(cross(N,X)), N);
  }

  static constexpr inline float operator "" _degrees(long double d) noexcept {
    return float(d * M_PI / 180);
  }

  // input: azimuth & elevation
  static inline vec3 spherical_to_cartesian(vec2 s) {
    auto cy = std::cos(s.y);
    return vec3{
      std::cos(s.x) * cy,
      std::sin(s.y),
      std::sin(s.x) * cy
    };
  }

  static inline vec2 cartesian_to_spherical(vec3 c) {
    float azimuth = std::atan2(c.z, c.x);
    if (azimuth < 0.0f)
      azimuth = tau + azimuth;
    return vec2(azimuth, std::asin(c.y));
  }

  template <typename T> static inline T radians_to_degrees(T radians) {
#pragma warning( push )
#pragma warning( disable : 4305 )
    return radians * T(180.0 / M_PI);
#pragma warning( pop )
  }

  template <typename T> static inline T degrees_to_radians(T degrees) {
#pragma warning( push )
#pragma warning( disable : 4305 )
    return degrees * T(M_PI / 180.0);
#pragma warning( pop )
  }

  using std::pow;

  template <int N> static inline float pow(float x) {    
    return N > 0  ? exp(x * log(N))
         : N == 0 ? 1.0f
                  : 1 / exp(x * log(N));
  }

  struct polar {
    float r, phi;
    vec2 to_disc() const {
      return vec2(
        r * std::cos(phi),
        r * std::sin(phi)
      );
    }
    static inline polar from_disc(vec2 xy) {
      return polar{ sqrt(xy.x * xy.x + xy.y * xy.y), atan2(xy.y, xy.x) };
    }
  };

  static inline float cos2sin(float x) noexcept { 
    return sqrt(std::max(0.f, 1.f - x*x)); 
  }

  static inline float sin2cos(float x) noexcept { 
    return sqrt(std::max(0.f, 1.f - x*x)); 
  }
}