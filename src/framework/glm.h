#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/detail/type_half.hpp>

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

  static vec3 perpendicular(const vec3 & v) {
    vec3 a = abs(v);
    float m = std::min(std::min(a.x, a.y), a.z);
    return normalize(
        (m == a.x) ? cross(v, vec3(1, 0, 0))
      : (m == a.y) ? cross(v, vec3(0, 1, 0))
                   : cross(v, vec3(0, 0, 1))
    );
  }
}