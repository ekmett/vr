#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/detail/type_half.hpp>

// helpers so that glm types can be dumped to spdlog
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
}