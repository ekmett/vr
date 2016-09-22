#pragma once

#include "std.h"
#include <limits>
#include <utility> // integer_sequence

#define _USE_MATH_DEFINES
#include <math.h>

namespace framework {
  namespace math_constexpr {

    inline constexpr uint32_t triangular(uint32_t n) {
      return n*(n+1)/2;
    }

    inline constexpr uint64_t triangular(uint64_t n) {
      return n*(n+1)/2;
    }

    inline constexpr int abs(int m) {
      return m < 0 ? -m : m;
    }

    inline constexpr float abs(float m) {
      return m < 0 ? -m : m;
    }

    template <typename T> inline constexpr T square(T x) {
      return x*x;
    }

    template <typename T> inline constexpr T cube(T x) {
      return x*x*x;
    }

    namespace detail {
      static constexpr float epsilon = 0.00001f; // largest legal FLT_EPSILON

      // Newton-Raphson
      inline constexpr float sqrt_step(float x, float curr, float prev) {
        return abs(curr - prev) < epsilon ? curr : sqrt_step(x, 0.5f * (curr + x / curr), curr);
      }

      // [triple angle formula](https://proofwiki.org/wiki/Triple_Angle_Formulas/Sine)
      inline constexpr float sin_step(float x) {
        return x < epsilon ? x : 3 * sin_step(x / 3.f) - 4 * cube(sin_step(x / 3.f));
      }

      // [triple angle formula](https://proofwiki.org/wiki/Triple_Angle_Formulas#Triple_Angle_Formula_for_Hyperbolic_Sine)
      inline constexpr float sinh_step(float x) {
        return x < epsilon ? x : 3 * sinh_step(x / 3.f) + 4 * cube(sinh_step(x / 3.f));
      }

      template <typename T> static inline constexpr T mid(T l, T h) {
        return l + (h - l) / 2;
      }

      // assumes l <= h, p(h)
      template <typename T, typename P> static inline constexpr T search(P p, T l, T h) {
        return l == h       ? l
             : p(mid(l, h)) ? search(p, l, mid(l, h))
                            : search(p, mid(l, h) + 1, h);
      }

      struct isqrt_test {
        const size_t n;
        constexpr const bool operator()(size_t i) const noexcept { return (i+1)*(i+1) > n; }
      };
    }

    inline constexpr float pow(float base, int exponent) {
      return exponent <  0 ? 1.f / pow(base, -exponent) 
           : exponent == 0 ? 1.f 
                           : base * pow(base, exponent - 1);
    }

    inline constexpr float sqrt(float x) {
      return x >= 0 && x < std::numeric_limits<float>::infinity() ? detail::sqrt_step(x, x, 0) : std::numeric_limits<float>::quiet_NaN();
    }
  
    inline constexpr size_t isqrt(size_t n) {
      return detail::search(detail::isqrt_test{ n }, size_t(0), n);
    }

    inline constexpr float sin(float x) {
      return detail::sin_step(x < 0 ? float(M_PI) - x : x);
    }

    inline constexpr float cos(float x) {
      return sin(float(M_PI_2) - x);
    }

    inline constexpr float tan(float x) {
      return sin(x) / cos(x);
    }

    inline constexpr float sinh(float x) {
      return x < 0 ? -detail::sinh_step(-x) : detail::sinh_step(x);
    }

    inline constexpr float cosh(float x) {
      return sqrt(1 + square(sinh(x)));
    }

    inline constexpr float tanh(float x) {
      return sinh(x) / cosh(x);
    }

    inline int constexpr factorial_power(int x, int n, int h = 1) {
      return (n > 0) ? x * factorial_power(x - h, n - 1, h) : 1;
    }

    inline int constexpr factorial(int x, int h = 1) {
      return (x > 1) ? x * factorial(x - h, h) : 1;
    }

    inline constexpr float K(int l, int m) {
      return sqrt((2 * l + 1) / (4 * float(M_PI) * factorial_power(l + abs(m), abs(m) + abs(m), 1)));
    }

    inline constexpr float legendre(int l, int m, float x) {
      return l == m + 1 ? x * (2 * m + 1) * legendre(m, m, x)
           : l == m     ? pow(-1, m) * factorial(2 * m - 1, 2) * pow(1 - x*x, m / 2)
                        : (x * (2 * l - 1) * legendre(l - 1, m, x) - (l + m - 1) * legendre(l - 2, m, x)) / (l - m);
    }
    
    inline constexpr float spherical_harmonic(int l, int m, float theta, float phi) {
      return m > 0 ? sqrt(2) * K(l, m) * cos(m*phi) * legendre(l, m, cos(theta))
           : m < 0 ? sqrt(2) * K(l, m) * sin(-m*phi) * legendre(l, -m, cos(theta))
                   : K(l, m) * legendre(l, 0, cos(theta));
    }
  }
}