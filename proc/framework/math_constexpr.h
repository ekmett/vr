#pragma once
#include <limits>
#define _USE_MATH_DEFINES
#include <math.h>

namespace framework {
  namespace math_constexpr {

    int constexpr abs(int m) {
      return m < 0 ? -m : m;
    }

    namespace detail {
      static constexpr float epsilon = 0.00001f;

      // Newton-Raphson
      float constexpr sqrt_step(float x, float curr, float prev) {
        return abs(curr - prev) < epsilon ? curr : sqrt_step(x, 0.5f * (curr + x / curr), curr);
      }

      // [triple angle formula](https://proofwiki.org/wiki/Triple_Angle_Formulas/Sine)
      float constexpr sin_step(float x) {
        return x < epsilon ? x : 3 * sin_step(x / 3.f) - 4 * cube(sin_step(x / 3.f));
      }

      // [triple angle formula](https://proofwiki.org/wiki/Triple_Angle_Formulas#Triple_Angle_Formula_for_Hyperbolic_Sine)
      float constexpr sinh_step(float x) {
        return x < epsilon ? x : 3 * sinh_step(x / 3.f) + 4 * cube(sinh_step(x / 3.f));
      }
    }

    template <typename T> T constexpr square(T x) {
      return x*x;
    }

    template <typename T> T constexpr cube(T x) {
      return x*x*x;
    }

    float constexpr pow(float base, int exponent) {
      return exponent <  0 ? 1.f / pow(base, -exponent) 
           : exponent == 0 ? 1.f 
           : exponent == 1 ? base 
                           : base * pow(base, exponent - 1);
    }

    float constexpr sqrt(float x) {
      return x >= 0 && x < std::numeric_limits<float>::infinity() ? detail::sqrt_step(x, x, 0) : std::numeric_limits<float>::quiet_NaN();
    }

    float constexpr sin(float x) {
      return detail::sin_step(x < 0 ? M_PI - x : x);
    }

    float constexpr cos(float x) {
      return sin(M_PI_2 - x);
    }

    float constexpr tan(float x) {
      return sin(x) / cos(x);
    }

    float constexpr sinh(float x) {
      return x < 0 ? -detail::sinh_step(-x) : detail::sinh_step(x);
    }

    float constexpr cosh(float x) {
      return sqrt(1 + square(sinh(x)));
    }

    float constexpr tanh(float x) {
      return sinh(x) / cosh(x);
    }

    int constexpr factorial_power(int x, int n, int h = 1) {
      return (n > 0) ? x * factorial_power(x - h, n - 1, h) : 1;
    }

    int constexpr factorial(int x, int h = 1) {
      return (x > 1) ? x * factorial(x - h, h) : 1;
    }

    float constexpr K(int l, int m) {
      return sqrt((2 * l + 1) / (4 * float(M_PI) * factorial_power(l + abs(m), abs(m) + abs(m), 1)));
    }

    float constexpr legendre(int l, int m, float x) {
      return l == m + 1 ? x * (2 * m + 1) * legendre(m, m, x)
           : l == m     ? pow(-1, m) * factorial(2 * m - 1, 2) * pow(1 - x*x, m / 2)
                        : (x * (2 * l - 1) * legendre(l - 1, m, x) - (l + m - 1) * legendre(l - 2, m, x)) / (l - m);
    }
    
    float constexpr spherical_harmonic(int l, int m, float theta, float phi) {
      return m > 0 ? sqrt(2) * K(l, m) * cos(m*phi) * legendre(l, m, cos(theta))
           : m < 0 ? sqrt(2) * K(l, m) * sin(-m*phi) * legendre(l, -m, cos(theta))
                   : K(l, m) * legendre(l, 0, cos(theta));
    }

    static const float test = spherical_harmonic(1, 2, 3, 4);
  }
}