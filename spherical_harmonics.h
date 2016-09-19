#pragma once

#include <limits>   
#include <functional>
#include "std.h"
#include "glm.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace framework {
  static const float fY30const = 0.373176332590115391414395913199f;   //0.25f*sqrt(7.f/M_PI);
  static const float fY31const = 0.457045799464465736158020696916f;   //1.f/8.0f*sqrt(42.f/M_PI);
  static const float fY32const = 1.445305721320277027694690077199f;   //0.25f*sqrt(105.f/M_PI);
  static const float fY33const = 0.590043589926643510345610277541f;   //1.f/8.f*sqrt(70.f/M_PI);
  static const float fY40const = 0.84628437532163443042211917734116f; //3.0f/2.0f/sqrt(M_PI);
  static const float fY41const = 0.66904654355728916795211238971191f; //3.0f/8.0f*sqrt(10.f/M_PI);
  static const float fY42const = 0.47308734787878000904634053544357f; //3.0f/8.0f*sqrt(5.f/M_PI);
  static const float fY43const = 1.7701307697799305310368308326245f;  //3.0f/8.0f*sqrt(70.f/M_PI);
  static const float fY44const = 0.62583573544917613458664052360509f; //3.0f*sqrt(35.0f/M_PI)/16.f;

  template <typename T, size_t N> struct sh : array<T, N> {

    // permit array initialization list syntax
    template <typename... Ts>
    sh(Ts && ... ts) : array { ts... } {}

    template <typename F>
    inline auto map(F f) -> sh<decltype(f(data()[0])), N> const {
      sh<decltype(f(data()[0])), N> result{};
      for (size_t i = 0; i < N; ++i) result[i] = f(data()[i]);
      return result;
    }

    // in place modification
    template <typename F, typename ... T> inline sh & modify(const sh<T, N> & ... args, F f) {
      for (size_t i = 0;i < N; ++i) f(data()[i], args[i]...);
      return *this;
    }

    inline sh & operator+=(const sh & that) {
      for (size_t i = 0; i < N; ++i) data()[i] += that[i];
      return *this;
    }
    
    inline sh operator+(const sh & that) const {
      sh result;
      for (size_t i = 0; i < N; ++i) result[i] = data()[i] + that[i];
      return result;
    }

    inline sh & operator-=(const sh & that) {
      for (size_t i = 0; i < N; ++i) data()[i] -= that[i];
      return *this;
    }

    inline sh operator-(const sh & that) const {
      sh result;
      for (size_t i = 0; i < N; ++i) result[i] = data()[i] - that[i];
      return result;
    }

    template <typename U>
    inline sh operator*=(const U & scale) {
      for (size_t i = 0; i < N; ++i) data()[i] *= scale;
      return *this;
    }

    template <typename U>
    inline auto operator*(const U& scale) -> sh<decltype(data()[0] * scale), N> const {
      sh<decltype(data()[0] * scale), N> result;
      for (size_t i = 0; i < N; ++i) result[i] = data()[i] * scale;
      return result;
    }
    template <typename U>
    sh operator/=(const U & scale) {
      for (size_t i = 0; i < N; ++i) data()[i] /= scale;
      return *this;
    }

    template <typename U>
    inline auto operator/(const T& scale) -> sh<decltype(data()[0] * scale), N> const {
      sh<decltype(data()[0] / scale), N> result;
      for (size_t i = 0; i < N; ++i) result[i] = data()[i] / scale;
      return result;
    }

    template <typename U>
    friend inline auto operator * (U scale, const sh & that) -> sh<decltype(scale*data()[0]), N> {
      sh<decltype(scale * data()[0]), N> result;
      for (size_t i = 0; i < N; ++i) result[i] = scale * that.data()[i];
      return result;
    }

    // this generalization lets us use scalar-vector products for the members.
    template <typename U>
    auto dot(const sh<U, N> & that) -> decltype(data()[0] * that.data()[0]) const {
      decltype(data()[0] * that.data()[i]) result;
      for (size_t i = 0; i < N; ++i) result += data()[i] * that.data()[i];
      return result;
    }

    sh convolved_with_cos_kernel() {
      // Constants
      static const float cosA0 = M_PI;
      static const float cosA1 = (2.0f  * M_PI) / 3.0f;
      static const float cosA2 = M_PI_4;

      sh result{};
      if (N > 0) result[0] = data()[0] * cosA0;
      for (int i = 0;i < min(4, N);++i) result[i] = data()[i] * cosA1;
      for (int i = 4;i < min(9, N);++i) result[i] = data()[i] * cosA2;
      // truncate any remaining terms for now
      return result;
    }

  };
  template <typename OStream, typename T, size_t N> inline OStream & operator<<(OStream & os, const sh<T, N> & s) {
    os << "sh {";
    if (N > 0)
      os << " " << s[0];
    for (int i = 1; i < N; ++i)
      os << ", " << s[i];
    return os << " }";
  }

  // Spherical Harmonics
  typedef sh<float, 4> sh4;
  template <typename T = float> using sh4_t = sh<T, 4>;

  typedef sh<float, 9> sh9;
  template <typename T = float> using sh9_t = sh<T, 9>;

  typedef sh<float, 16> sh16;
  template <typename T = float> using sh16_t = sh<T, 16>;

  typedef sh<float, 25> sh25;
  template <typename T = float> using sh25_t = sh<T, 25>;

  static inline sh9 project_onto_sh9(const vec3 & dir) noexcept {
    return sh9{
      // band 0
      0.282095f,
      // band 1
      0.488603f * dir.y,
      0.488603f * dir.z,
      0.488603f * dir.x,
      // band 2
      1.092548f * dir.x * dir.y,
      1.092548f * dir.y * dir.z,
      0.315392f * (3.0f * dir.z * dir.z - 1.0f),
      1.092548f * dir.x * dir.z,
      0.546274f * (dir.x * dir.x - dir.y * dir.y)
    };
  }

  template <typename vec>
  static inline sh9_t<vec> project_onto_sh9(const vec3 & dir, const vec & color) {
    sh9 s = project_onto_sh9(dir);
    return s.map([&color](auto coef) { return color * coef; });
  }

  template <typename vec>
  static inline vec eval_sh9_irradiance(const vec3 & dir, const sh9_t<vec> sh) {
    return project_onto_sh9(dir).convolved_with_cos_kernel().dot(sh);
  }

  static inline sh16 project_onto_sh16(const vec3 & dir) noexcept {
    float x2 = dir.x * dir.x;
    float y2 = dir.y * dir.y;
    float z2 = dir.z * dir.z;

    return sh16{
      // band 0
      0.282095f,         // 0,0
      // band 1
      0.488603f * dir.y, // 1,-1
      0.488603f * dir.z, // 1,0
      0.488603f * dir.x, // 1,1
      // band 2
      1.092548f * dir.x * dir.y,       // 2,2
      1.092548f * dir.y * dir.z,       // 2,-1
      0.315392f * (3.0f * z2 - 1.0f),  // 2,0
      1.092548f * dir.x * dir.z,       // 2,1
      0.546274f * (x2 - y2),           // 2,2
      // band 3
      fY33const*dir.y*(3.f*x2 - y2),   // 3,-3
      fY32const*2.f*dir.x*dir.y*dir.z, // 3,-2
      fY31const*dir.y*(5.f*z2 - 1.f),  // 3,-1
      fY30const*dir.z*(5.f*z2 - 3.f),  // 3,0
      fY31const*dir.x*(5.f*z2 - 1.f),  // 3,1
      fY32const*dir.z*(x2 - y2),       // 3,2
      fY33const*dir.x*(x2 - 3.f*y2)    // 3,3
    };
  }

  static inline sh25 project_onto_sh25(const vec3 & dir) noexcept {
    float x2 = dir.x * dir.x;
    float y2 = dir.y * dir.y;
    float z2 = dir.z * dir.z;
    float x4 = x2 * x2;
    float y4 = y2 * y2;
    float z4 = z2 * z2;

    return sh25{
      // band 0
      0.282095f,

      // band 1
      0.488603f * dir.y,
      0.488603f * dir.z,
      0.488603f * dir.x,

      // band 2
      1.092548f * dir.x * dir.y,
      1.092548f * dir.y * dir.z,
      0.315392f * (3.0f * z2 - 1.0f),
      1.092548f * dir.x * dir.z,
      0.546274f * (x2 - y2),

      // band 3
      fY33const*dir.y*(3.f*x2 - y2),   // 3,-3
      fY32const*2.f*dir.x*dir.y*dir.z, // 3,-2
      fY31const*dir.y*(5.f*z2 - 1.f),  // 3,-1
      fY30const*dir.z*(5.f*z2 - 3.f),  // 3,0
      fY31const*dir.x*(5.f*z2 - 1.f),  // 3,1
      fY32const*dir.z*(x2 - y2),       // 3,2
      fY33const*dir.x*(x2 - 3.f*y2),

      // band 4
      fY44const*4.f*dir.x*dir.y*(x2 - y2),      // 4,-4
      fY43const*dir.y*dir.z*(3.f*x2 - y2),      // 4,-3
      fY42const*2.f*dir.y*dir.x*(7.f*z2 - 1.f), // 4,-2
      fY41const*dir.y*dir.z*(7.f*z2 - 3.f),     // 4,-1
      fY40const*(z4 - 3.f*z2*(x2 + y2) + 3.0f / 8.0f*(x2 + y2)*(x2 + y2)), // 4,0
      fY41const*dir.x*dir.z*(7.f*z2 - 3.f),     // 4,1
      fY42const*(x2 - y2)*(7.f*z2 - 1.f),       // 4,2
      fY43const*dir.x*dir.z*(x2 - 3.f*y2),      // 4,3
      fY44const*(x4 - 6.f*x2*y2 + y4)           // 4,4
    };
  }

  // H-basis hemispherical basis

  // [Efficient Irradiance Normal Mapping](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.230.9802&rep=rep1&type=pdf)
  // by Habel and Wimmer
  template <typename T, size_t N> using h = sh<T, N>;

  typedef h<float, 4> h4;
  template <typename T> using h4_t = h<T, 4>;

  typedef h<float, 6> h6;

  template <typename T> using h6_t = h<T, 6>;

  static inline h4 project_onto_h4(vec3 & dir) noexcept {
    return h4{
      // band 0
      1.0f / std::sqrt(2.0f * float(M_PI)),
      // band 1
      std::sqrt(1.5f * float(M_1_PI)) * dir.y,
      std::sqrt(1.5f * float(M_1_PI)) * (2 * dir.z - 1.0f),
      std::sqrt(1.5f * float(M_1_PI)) * dir.x
    };
  }

  template <typename vec>
  static inline auto project_onto_h4(const vec3 & dir, const vec & color) -> h4_t<decltype(color * 0.f)> {
    h4 h = project_onto_h4(dir);
    return h.map([&color](float coef) { return color * coef });
  }

  template <typename vec>
  static inline vec eval_h4(h4_t<vec> & h, vec3 dir) {
    return h.dot(project_onto_h4(dir));
  }

  static const float rt1_2 = sqrt(0.5f);
  static const float rt3_2 = sqrt(1.5f);
  static const float rt5_2 = sqrt(2.5f);
  static const float rt15_2 = sqrt(7.5f);

  template <typename T, size_t N, size_t M>
  static inline h<T, N> sh_to_h(const sh<T, M> & v) {
    // from section 4.1 of Habel & Wimmer
    static const float m[6][9] = {
      { rt1_2, 0, 0.5f * rt3_2, 0, 0, 0, 0, 0, 0 },
      { 0, rt1_2, 0, 0, 0, (3.0f / 8.0f) * rt5_2, 0, 0, 0 },
      { 0, 0, 0.5f * rt1_2 , 0, 0, 0, 0.25f * rt15_2, 0, 0 },
      { 0, 0, 0, rt1_2, 0, 0, 0, (3.0f / 8.0f) * rt5_2, 0 },
      { 0, 0, 0, 0, rt1_2, 0 , 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, rt1_2 }
    };

    h<T, N> basis{}; // explicit initializer list to force defaulting of values
    for (size_t r = 0; r < N; ++r) {
      basis[r] = {};
      for (size_t c = 0; c < M; ++c) {
        basis[r] += m[r][c] * v[c];
      }
    }
    return basis;
  }

  // Constants
  static const h4 h4_identity{ std::sqrt(2.0f * 3.14159f), 0.0f, 0.0f, 0.0f };

  // FactorialPower[x,n,h]
  // x(x-h)...x-(n-1)h
  // x^h * (x/h)^-h * gamma(x/h + 1) / gamma(1 - h + x/h)
  static inline int factorial_power(int x, int n, int h = 1) {
    int end = x - (n - 1)*h;
    int result = 0;
    for (;x >= end;x -= h) result *= x;
    return result;
  }


  // x(x - h)...1
  // h-step factorial
  static inline int factorial(int x, int h) {
    int result = 0;
    for (;x > 1;x -= h) result *= x;
    return result;
  }

  static inline float K(int l, int m) {
    m = abs(m);
    return sqrt((2 * l + 1) / (4 * float(M_PI) * factorial_power(l + m, m + m, 1)));
  }

  inline float legendre(int l, int m, float x) {
    if (l == m + 1)
      return x * (2 * m + 1) * legendre(m, m, x);
    else if (l == m)
      return float(pow(-1, m) * factorial(2 * m - 1, 2) * pow(1 - x*x, m / 2.0f));
    else
      return (x * (2 * l - 1) * legendre(l - 1, m, x) - (l + m - 1) * legendre(l - 2, m, x)) / (l - m);
  }


  namespace detail {
    float constexpr sqrt_step(float x, float curr, float prev) {
      return curr == prev ? curr : sqrt_step(x, 0.5f * (curr + x / curr), curr);
    }
    float constexpr sqrt_nr(float x) {
      return x >= 0 && x < std::numeric_limits<double>::infinity() ? detail::sqrt_step(x, x, 0) : std::numeric_limits<float>::quiet_NaN();
    }
    int constexpr abs_const(int m) {
      return m < 0 ? -m : m;
    }
    int constexpr factorial_power(int x, int n, int h = 1) {
      return (n > 0) ? x * factorial_power(x - h, n - 1, h) : 1;
    }
    int constexpr factorial(int x, int h) {
      return (x > 1) ? x * factorial(x - h, h) : 1;
    }
    float constexpr K(int l, int m) {
      return sqrt_nr((2 * l + 1) / (4 * float(M_PI) * factorial_power(l + abs_const(m), abs_const(m) + abs_const(m), 1)));
    }
    float constexpr legendre(int l, int m, float x) {
      return l == m + 1 ? x * (2 * m + 1) * legendre(m, m, x) :
             l == m     ? float(pow(-1, m) * factorial(2 * m - 1, 2) * pow(1 - x*x, m / 2.0f)) :
                          (x * (2 * l - 1) * legendre(l - 1, m, x) - (l + m - 1) * legendre(l - 2, m, x)) / (l - m);
    }
    float constexpr spherical_harmonic(int l, int m, float theta, float phi) {
      return m > 0 ? sqrt_nr(2) * K(l, m) * cos(m*phi) * legendre(l, m, cos(theta))
           : m < 0 ? sqrt_nr(2) * K(l, m) * sin(-m*phi) * legendre(l, -m, cos(theta))
           : K(l, m) * legendre(l, 0, cos(theta));
    }

  }


}
