#pragma once

#include "math.h"
#include "sampling_sobol.h"
#include "sampling_hammersley.h"

namespace framework {
  // Sample uniformly from the surface of a sphere using 
  // [Archimedes' Hat-Box Theorem](http://mathworld.wolfram.com/ArchimedesHat-BoxTheorem.html).
  vec3 sample_sphere(vec2 uv) noexcept;
  static constexpr const float sample_sphere_pdf = float(0.25 / M_PI);

  // Sample, cosine weighted, from the surface of a sphere
  vec3 sample_sphere_cos(vec2 uv, float * pdf = nullptr) noexcept;

  // [Random sampling inside a sphere](http://6degreesoffreedom.co/circle-random-sampling/)
  vec3 sample_ball(vec3 uvw) noexcept;
  static constexpr const float sample_ball_pdf = float(0.75 / M_PI);

  template <typename T> 
  inline T sample_triangle(T A, T B, T C, vec2 uv) noexcept {
    float su = sqrt(uv.x);
    return C + (1.f - su) * (A - C) + (uv.y * su) * (B - C);
  }

  namespace detail {
    // Optimal sorting of three elements
    // based on the vector-algorithms implementation by Dan Doel
    template <typename T>
    void sort3(T & a0, T & a1, T & a2) {
      if (a0 > a1) {
        if (a0 > a2) {
          if (a1 > a2) {
            std::swap(a0, a2); // a2,a1,a0
          } else {           
            // use a temporary and std::move?
            std::swap(a0, a1); // a1,a0,a2
            std::swap(a1, a2); // a1,a2,a0
          }
        } else {
          std::swap(a0, a1);   // a1,a0,a2
        }
      } else if (a1 > a2) {
        if (a0 > a2) {
          // use a temporary and std::move?
          std::swap(a1, a2);  // a0,a2,a1
          std::swap(a0, a1);  // a2,a0,a1
        } else {
          std::swap(a1, a2)   // a0,a2,a1
        }
      }                       // a0,a1,a2
    }
  }

  // [Heron's formula](https://en.wikipedia.org/wiki/Heron%27s_formula) for computing an area.
  // [Miscalculating Area and Angles of a Needle-like Triangle](https://people.eecs.berkeley.edu/~wkahan/Triangle.pdf) by William Kahan
  template <typename T>
  inline float sample_triangle_pdf(const T & A, const T & B, const T & C) noexcept {
    float a = length(A - B);
    float b = length(B - C);
    float c = length(C - A);
    sort3(c, b, a); // c <= b <= a
    return 4.0f / sqrt((a + (b + c)) * (c - (a - b)) * (c + (a - b)) * (a + (b - c))); // the parens matter
  }

  // reference implementation
  inline float sample_triangle_pdf_naive(vec3 A, vec3 B, vec3 C) noexcept {
    vec3 x = A - C, y = B - C;
    return 2.0f / sqrt(square(x.y*y.z - x.z*y.y) + square(x.z*y.x - x.x*y.z) + square(x.x*y.y - x.y*y.x));
  }

  float sample_triangle_pdf(vec3 A, vec3 B, vec3 C) noexcept;

  // Sample from the surface of a hemisphere using concentric disc sampling.
  vec3 sample_hemisphere(vec2 uv) noexcept;
  static constexpr const float sample_hemisphere_pdf = float(0.5 / M_PI);

  // Sample from a cosine weighted hemisphere using concentric disc sampling.
  vec3 sample_hemisphere_cos(vec2 uv, float * pdf = nullptr) noexcept;

  // Sample from a hemisphere weighted with a phong-like power cosine. 
  // sample_hemisphere and sample_hemisphere_cos are special cases
  // TODO: concentric discs
  vec3 sample_hemisphere_power_cos(vec2 uv, float exponent, float * pdf = nullptr) noexcept;

  // Sample from a directional cone using Archimedes hat box.
  vec3 sample_cone(vec2 uv, float cosThetaMax) noexcept;
  inline float constexpr sample_cone_pdf(float cosThetaMax) noexcept {
    return 1.f / (tau * (1.f - cosThetaMax));
  }

  // Direct sampling from an annulus on a disc using polar projection
  vec2 sample_annulus(vec2 uv, float r_min, float r_max) noexcept;
  static constexpr float sample_annulus_pdf(float r_min, float r_max) noexcept {
    return 2 / (r_max*r_max - r_min*r_min);
  }

  // draw samples from polar coordinates drawn uniformly from the disc of radius 1
  polar sample_polar(vec2 uv) noexcept;
  vec2 unsample_polar(polar p) noexcept;

  // sample uniformly from a disc of radius 1.
  vec2 sample_disc(vec2 uv) noexcept;
  vec2 sample_disc(vec2 uv, float n, float blend_weight) noexcept;
  static constexpr const float sample_disc_pdf = float(M_1_PI);
  vec2 unsample_disc(vec2 xy) noexcept;

  // ggx sample and pdf calculation
  vec3 sample_ggx(float roughness, vec3 N, mat3 TtoW, vec2 uv, vec3 V, float * pdf = nullptr) noexcept;

  // used for debugging
  void sampling_debug_window(bool * open, mat4 V);
};