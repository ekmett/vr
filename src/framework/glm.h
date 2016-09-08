#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/detail/type_half.hpp>


#define _USE_MATH_DEFINES
#include <math.h>  // M_PI


namespace glm {
  template <typename T, glm::precision P, typename ostream> ostream & operator << (ostream & os, const tvec1<T, P> & v) {
    return os << "[" << v.x << "]";
  }
  template <typename T, glm::precision P, typename ostream> ostream & operator << (ostream & os, const tvec2<T, P> & v) {
    return os << "[" << v.x << "," << v.y << "]";
  }
  template <typename T, glm::precision P, typename ostream> ostream & operator << (ostream & os, const tvec3<T, P> & v) {
    return os << "[" << v.x << "," << v.y << "," << v.z << "]";
  }
  template <typename T, glm::precision P, typename ostream> ostream & operator << (ostream & os, const tvec4<T,P> & v) {
    return os << "[" << v.x << "," << v.y << "," << v.z << "," << v.w << "]";
  }
}

namespace framework {
  using namespace glm;

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
    return radians * T(180.0 / M_PI);
  }

  template <typename T> T degrees_to_radians(T degrees) {
    return degrees * T(M_PI / 180.0);
  }
}