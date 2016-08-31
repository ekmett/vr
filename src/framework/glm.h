#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>

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