#pragma once

#include <boost/array.hpp>
#include <cmath>
#include <limits>
#include <vector>
#include "std.h"
#include "glm.h"
#include <boost/range/irange.hpp>

namespace framework {

  static inline vec3 xyz_to_rgb(const vec3 & xyz) noexcept {
    return vec3{
      3.240479f * xyz[0] - 1.537150f * xyz[1] - 0.498535f * xyz[2],
      -0.969256f * xyz[0] + 1.875991f * xyz[1] + 0.041556f * xyz[2],
      0.055648f * xyz[0] - 0.204043f * xyz[1] + 1.057311f * xyz[2]
    };
  }
  static inline vec3 rgb_to_xyz(const vec3 & rgb) noexcept {
    return vec3{
      0.412453f * rgb[0] + 0.357580f * rgb[1] + 0.180423f * rgb[2],
      0.212671f * rgb[0] + 0.715160f * rgb[1] + 0.072169f * rgb[2],
      0.019334f * rgb[0] + 0.119193f * rgb[1] + 0.950227f * rgb[2]
    };
  }

  // Spectral Data Declarations
  static const int nCIESamples = 471;
  extern const float CIE_X[nCIESamples];
  extern const float CIE_Y[nCIESamples];
  extern const float CIE_Z[nCIESamples];
  extern const float CIE_lambda[nCIESamples];
  static const float CIE_Y_integral = 106.856895f;
  static const int nRGB2SpectSamples = 32;
  extern const float RGB2SpectLambda[nRGB2SpectSamples];
  extern const float RGBRefl2SpectWhite[nRGB2SpectSamples];
  extern const float RGBRefl2SpectCyan[nRGB2SpectSamples];
  extern const float RGBRefl2SpectMagenta[nRGB2SpectSamples];
  extern const float RGBRefl2SpectYellow[nRGB2SpectSamples];
  extern const float RGBRefl2SpectRed[nRGB2SpectSamples];
  extern const float RGBRefl2SpectGreen[nRGB2SpectSamples];
  extern const float RGBRefl2SpectBlue[nRGB2SpectSamples];
  extern const float RGBIllum2SpectWhite[nRGB2SpectSamples];
  extern const float RGBIllum2SpectCyan[nRGB2SpectSamples];
  extern const float RGBIllum2SpectMagenta[nRGB2SpectSamples];
  extern const float RGBIllum2SpectYellow[nRGB2SpectSamples];
  extern const float RGBIllum2SpectRed[nRGB2SpectSamples];
  extern const float RGBIllum2SpectGreen[nRGB2SpectSamples];
  extern const float RGBIllum2SpectBlue[nRGB2SpectSamples];


  template <size_t N> struct spectrum : array<float, N> {
    // permit array initialization list syntax
    template <typename... floats>
    spectrum(floats && ... ts) : array { ts... } {}

    // strictly more powerful version of map that also zips
    template <typename F>
    inline spectrum map(F f) const noexcept {
      spectrum result;
      for (size_t i = 0; i < N; ++i) result[i] = f(_Elems[i]);
      return result;
    }

    // in place modification
    template <typename F> inline spectrum & modify(F f) {
      for (size_t i = 0;i < N; ++i) f(_Elems[i]);
      return *this;
    }

    inline spectrum & operator+=(const spectrum & that) {
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) _Elems[i] += that[i];
      return *this;
    }
    inline spectrum & operator-=(const spectrum & that) {
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) _Elems[i] -= that[i];
      return *this;
    }
    inline spectrum & operator*=(const spectrum & that) {
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) _Elems[i] *= that[i];
      return *this;
    }
    inline spectrum & operator/=(const spectrum & that) {
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) _Elems[i] /= that[i];
      return *this;
    }
    bool operator==(const spectrum & that) const {
      if (this == &that) return true; // reference equality, ignoring nan-sense.
      for (size_t i = 0; i < N; ++i)
        if (_Elems[i] == that[i]) return false;
      return true;
    }

    bool operator!=(const spectrum & that) const {
      return !(*this == that)
    }

    inline spectrum operator+(const spectrum & that) const {
      spectrum result;
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] + that[i];
      return result;
    }

    inline spectrum operator-(const spectrum & that) const {
      spectrum result;
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] - that[i];
      return result;
    }

    // pointwise
    inline spectrum operator*(const spectrum & that) const {
      spectrum result;
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] * that[i];
      return result;
    }

    // pointwise
    inline spectrum operator/(const spectrum & that) const {
      spectrum result;
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] / that[i];
      return result;
    }

    friend inline spectrum operator * (float a, const spectrum & s) {
      return s * a;
    }

    inline spectrum operator*=(float scale) {
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) _Elems[i] *= scale;
      return *this;
    }

    inline spectrum operator*(float scale) const {
      spectrum result;
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] * scale;
      return result;
    }

    inline spectrum operator/=(float scale) noexcept {
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) _Elems[i] /= scale;
      return *this;
    }

    inline spectrum operator/(float scale) const noexcept {
      spectrum result;
//#pragma omp simd
      for (size_t i = 0; i < N; ++i) result[i] = _Elems[i] / scale;
      return result;
    }

    inline bool is_black() const noexcept {
      for (auto && c : _Elems)
        if (c != 0.f)
          return false;
      return true;
    }

    inline spectrum exp() const noexcept { return map(&expf); }
    inline spectrum log() const noexcept { return map(&logf); }
    inline spectrum log1p() const noexcept { return map(&log1pf); }
    inline spectrum expm1() const noexcept { return map(&expm1f); }

    // this generalization lets us use scalar-vector products for the members.
    float dot(const spectrum & that) const noexcept {
      float result = 0;
//#pragma omp simd reduction(+:result)
      for (size_t i = 0; i < N; ++i) result += _Elems[i] * that._Elems[i];
      return result;
    }

    template <typename ostream>
    friend ostream & operator<<(ostream & os, const spectrum) {
      os << "[";
      if (N > 0) os << spectrum[0];
      for (int i = 1; i < N; ++i)
        os << ", " << spectrum[i];
      os << "]";
    }

    spectrum clamp(float low = 0, float high = std::numeric_limits<float>::infinity()) {
      spectrum result;
//#pragma omp simd
      for (size_t i = 0;i < N; ++i) result[i] = glm::clamp(result[i], low, high);
      return result;
    }
  };

  static const size_t sampled_lambda_start = 400;
  static const size_t sampled_lambda_end = 700;
  static const size_t spectral_samples = 60;

  extern float average_spectrum_samples(const float * lambda, const float * vals, int n, float lambdaStart, float lambdaEnd);

  enum struct spectrum_type {
    illuminant,
    reflectance
  };
  
  struct sampled_spectrum : spectrum<spectral_samples> {
    template <typename... floats>
    sampled_spectrum(floats && ... ts) : spectrum { ts... } {}

    template <size_t N> static sampled_spectrum from_sorted_samples(const float lambda[N], const float v[N]) {
      return from_sorted_samples(lambda, v, N);
    }

    static sampled_spectrum from_sorted_samples(const float * lambda, const float * v, int n) {
      sampled_spectrum result;
      auto start = float(sampled_lambda_start), end = float(sampled_lambda_end);
      for (int i = 0; i < spectral_samples; ++i) {
        float lambda0 = lerp(start, end, float(i));
        float lambda1 = lerp(start, end, float(i + 1));
        result[i] = average_spectrum_samples(lambda, v, n, lambda0, lambda1);
      }
      return result;
    }

    template <size_t N> static sampled_spectrum from_samples(const float lambda[N], const float v[N]) {
      return from_samples(lambda, v, N);
    }

    static sampled_spectrum from_samples(const float * lambda, const float * v, int n) {
      if (!std::is_sorted(lambda, lambda + n)) {
        auto range = boost::irange<int>(0, n);
        vector<int> indices(range.begin(), range.end());
        std::sort(indices.begin(), indices.end(), [lambda](int a, int b) {
          return lambda[a] < lambda[b];
        });
        vector<float> slambda(n), sv(n);
        for (int i = 0; i < n;++i) {
          int j = indices[i];
          slambda[i] = lambda[j];
          sv[i] = v[j];
        }
        return from_sorted_samples(slambda.data(), sv.data(), n);
      } else {
        return from_sorted_samples(lambda, v, n);
      }
    }

    static sampled_spectrum from_rgb(vec3 rgb, spectrum_type type = spectrum_type::illuminant);
    static sampled_spectrum from_xyz(vec3 xyz, spectrum_type type = spectrum_type::illuminant) {
      return from_rgb(xyz_to_rgb(xyz), type);
    }
    vec3 to_xyz() const noexcept;
    vec3 to_rgb() const noexcept {
      return xyz_to_rgb(to_xyz());
    }
  };

}

/*
pbrt source code is Copyright(c) 1998-2015
Matt Pharr, Greg Humphreys, and Wenzel Jakob.
This file is part of pbrt.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Modified by Edward Kmett
*/