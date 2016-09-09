#pragma once

namespace framework {
  template <typename N, typename T> static inline constexpr T plural(N n, T s, T p = s + "s") {
    return (n == 1) ? s : p;
  }
  template <typename N, typename T> static inline constexpr T & plural(N n, T & s, T & p = s + "s") {
    return (n == 1) ? s : p;
  }
}