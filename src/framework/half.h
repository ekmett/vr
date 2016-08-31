#pragma once

#include "framework/glm.h"
#include <glm/detail/type_half.hpp>

namespace framework {
  struct half {
    half() : data(0) {}
    half(float f) : data(glm::detail::toFloat16(f)) {}
    half & operator = (float f) {
      data = glm::detail::toFloat16(f);
    }
    operator float() const { return glm::detail::toFloat32(data); }
    uint16_t data;
  };

  template <typename ostream> ostream & operator << (ostream & os, half h) {
    return os << float(h);
  }

  template <typename istream> istream & operator >> (istream & is, half & h) {
    float f;
    is >> f;
    h = f;
    return is;
  }

  half operator "" _half(long double f) {
    return half(float(f));
  }

}