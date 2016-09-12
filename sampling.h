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