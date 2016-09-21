#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/detail/type_half.hpp>
#include <glm/gtc/vec1.hpp>

namespace glm {
  // helpers so that glm types can be dumped to spdlog
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

  template <size_t N> struct vec {};
  template <> struct vec<1> { typedef vec1 type; };
  template <> struct vec<2> { typedef vec2 type; };
  template <> struct vec<3> { typedef vec3 type; };
  template <> struct vec<4> { typedef vec4 type; };

  template <size_t M, size_t N = M> struct mat {};
  template <> struct mat<2, 4> { typedef mat2x4 type; };
  template <> struct mat<3, 4> { typedef mat3x4 type; };
  template <> struct mat<4, 4> { typedef mat4 type; };
  template <> struct mat<2, 3> { typedef mat2x3 type; };
  template <> struct mat<3, 3> { typedef mat3 type; };
  template <> struct mat<4, 3> { typedef mat4x2 type; };
  template <> struct mat<2, 2> { typedef mat2 type; };
  template <> struct mat<3, 2> { typedef mat3x2 type; };
  template <> struct mat<4, 2> { typedef mat4x2 type; };
}