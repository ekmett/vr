#pragma once

#include <functional>
#include "std.h"
#include "glm.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace framework {

  template <typename T, size_t N> struct sh : array<T, N> {

    // permit array initialization list syntax
    template <typename... Ts>
    sh(Ts && ... ts) : array { ts... } {}

    template <typename F>
    inline auto map(F f) -> sh<decltype(f(_Elems[0])), N> const {
      sh<decltype(f(_Elems[0])), N> result{};
      for (size_t i = 0; i < N; ++i) result[i] = f(_Elems[i]);
      return result;
    }

    //template <typename F, typename ... T>
    //inline auto map(const sh<T, N> & ... args, F f) -> sh<decltype(f(_Elems[0], args[0]...)), N> const {
    //  sh<decltype(f(_Elems[0], args[0]...)), N> result{};
    //  for (size_t i = 0; i < N; ++i) result[i] = f(_Elems[i], args[i]...);
    //  return result;
    //}

    // in place modification
    template <typename F, typename ... T> inline sh & modify(const sh<T, N> & ... args, F f) {
      for (size_t i = 0;i < N; ++i) f(_Elems[i], args[i]...);
      return *this;
    }

    inline sh & operator+=(const sh & that) {
      for (size_t i = 0; i < N; ++i) _Elems[i] += that[i];
      return *this;
    }
    inline sh operator+(const sh & that) const {
      sh result;
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] + that[i];
      return result;
    }
    inline sh & operator-=(const sh & that) {
      for (size_t i = 0; i < N; ++i) _Elems[i] -= that[i];
      return *this;
    }
    inline sh operator-(const sh & that) const {
      sh result;
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] - that[i];
      return result;

    }
    template <typename U>
    inline sh operator*=(const U & scale) {
      for (size_t i = 0; i < N; ++i) _Elems[i] *= scale;
      return *this;
    }

    template <typename U>
    inline auto operator*(const U& scale) -> sh<decltype(_Elems[0] * scale), N> const {
      sh<decltype(_Elems[0] * scale), N> result;
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] * scale;
      return result;
    }
    template <typename U>
    sh operator/=(const U & scale) {
      for (size_t i = 0; i < N; ++i) _Elems[i] /= scale;
      return *this;
    }

    template <typename U>
    inline auto operator/(const T& scale) -> sh<decltype(_Elems[0] * scale), N> const {
      sh<decltype(_Elems[0] / scale), N> result;
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] / scale;
      return result;
    }

    template <typename U>
    friend inline auto operator * (U scale, const sh & that) -> sh<decltype(scale*_Elems[0]), N> {
      sh<decltype(scale * _Elems[0]), N> result;
      for (size_t i = 0; i < N; ++i) result[i] = scale * that._Elems[i];
      return result;
    }

    // this generalization lets us use scalar-vector products for the members.
    template <typename U>
    auto dot(const sh<U, N> & that) -> decltype(_Elems[0] * that._Elems[0]) const {
      decltype(_Elems[0] * that._Elems[i]) result;
      for (size_t i = 0; i < N; ++i) result += _Elems[i] * that._Elems[i];
      return result;
    }

    sh convolved_with_cos_kernel() {
      // Constants
      static const float cosA0 = M_PI;
      static const float cosA1 = (2.0f  * M_PI) / 3.0f;
      static const float cosA2 = M_PI_4;

      sh result{};
      if (N > 0) result[0] = _Elems[0] * cosA0;
      for (int i = 0;i < min(4, N);++i) result[i] = _Elems[i] * cosA1;
      for (int i = 4;i < min(9, N);++i) result[i] = _Elems[i] * cosA2;
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
    //static_assert(N <= 6);
    //static_assert(M <= 9);
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

}
/* Portions drawn from:  
  MJP's DX12 Sample Framework
  http://mynameismjp.wordpress.com/
  All code licensed under the MIT license
*/