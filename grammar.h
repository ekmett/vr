#pragma once

namespace framework {
  template <typename T> static inline constexpr T plural(uint64_t n, T s, T p) {
    return (n == 1) ? s : p;
  }
  template <typename T = std::string> static inline T plural(uint64_t n, T s) {
    return (n == 1) ? s : s + "s";
  }
}