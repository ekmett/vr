#pragma once

#include "math.h"
#include "sampling_sobol.h"
#include "sampling_hammersley.h"

namespace framework {

  // Sample uniformly from the surface of a sphere using 
  // [Archimedes' Hat-Box Theorem](http://mathworld.wolfram.com/ArchimedesHat-BoxTheorem.html).
  vec3 sample_sphere(vec2 uv) noexcept;
  static constexpr const float sample_sphere_pdf = float(0.25 / M_PI);

  // [Random sampling inside a sphere](http://6degreesoffreedom.co/circle-random-sampling/)
  inline vec3 sample_ball(vec3 uvw) noexcept {
    return sample_sphere(vec2(uvw)) *  cbrt(uvw.z);
  }  
  static constexpr const float sample_ball_pdf = float(0.75 / M_PI);

  // Sample from the surface of a hemisphere using concentric disc sampling. This produces less variance in jittered samples than Archimedes.
  vec3 sample_hemisphere(vec2 uv) noexcept;
  static constexpr const float sample_hemisphere_pdf = float(0.5 / M_PI);

  // sample from the surface of a cosine weighted hemisphere by mapping to the unit disc with concentric disc sampling and then projecting.
  vec3 sample_hemisphere_cos(vec2 uv) noexcept;

  // cos theta / pi
  inline float sample_hemisphere_cos_pdf(vec2 uv) noexcept { 
    return sqrt(std::max(0.f, 1.f - uv.x)) * float(M_1_PI);
  }

  // Archimedes hat box.
  vec3 sample_cone(vec2 uv, float cosThetaMax) noexcept;
  inline float constexpr sample_cone_pdf(float cosThetaMax) noexcept {
    return 1.f / (tau * (1.f - cosThetaMax));
  }

  extern vec2 sample_annulus(vec2 uv, float r_min, float r_max) noexcept;
  inline float constexpr sample_annulus_pdf(float r_min, float r_max) noexcept {
    return 2 / (r_max*r_max - r_min*r_min);
  }
  
  extern polar sample_polar(vec2 uv) noexcept;
  extern vec2 unsample_polar(polar p) noexcept;

  inline vec2 unsample_disc(vec2 xy) noexcept {
    return unsample_polar(polar::from_disc(xy));
  }

  inline vec2 sample_disc(vec2 uv, float n, float weight) noexcept {
    polar p = sample_polar(uv);
    float polygon_modifier = cos(pi / n) / cos(p.phi - (pi / n) * floor((n * p.phi + pi) / tau));
    p.r *= mix(1.f, polygon_modifier, weight);
    return p.to_disc();
  }

  static constexpr const float sample_disc_pdf = float(M_1_PI);

  float sample_ggx_pdf(float roughness, vec3 N, vec3 H, vec3 V) noexcept; // no need for L
  //--------------------------------------------------------------------------
  // Halton Low Discrepancy Sequence
  //--------------------------------------------------------------------------

  // may be useful if I add faure or halton
  namespace detail {
    // compute the base b expansion of k
    inline vector<int> b_ary(int k, int b) {
      vector<int> result(0);
      while (k >= b) {
        result.push_back(k % b);
        k = k / b;
      }
      return result;
    }
  }

  // used for debugging
  void sampling_debug_window(bool * open, mat4 V);

};

// The modification to include the polygon morphing modification for lens diaphram simulation from 
// ["CryEngine3 Graphics Gems" (slide 36)](http://www.crytek.com/download/Sousa_Graphics_Gems_CryENGINE3.pdf)
// by Tiago Sousa was drawn from [MJP's DX12 Sample Framework](http://mynameismjp.wordpress.com/) and 
// is licensed under the MIT license.