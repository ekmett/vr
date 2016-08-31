#include "framework/stdafx.h"
#include "skybox.h"
#include "framework/glm.h"
#include <cmath>
#include <algorithm>
#include "framework/spectrum.h"
#include "framework/sampling.h"

#include "ArHosekSkyModel.h"

static inline float angle_between(const glm::vec3 & x, const glm::vec3 & y) {
  return std::acosf(std::max(glm::dot(x,y), 0.00001f));
}

constexpr float operator "" _degrees(long double d) noexcept {
  return float(d * M_PI / 180.f);
}

static const float physical_sun_size = 0.27_degrees;
static const float cos_physical_sun_size = std::cos(physical_sun_size);
static const float fp16_scale = 0.0009765625f; // 2^-10 scaling factor to allow storing physical lights in fp16 floats

static float irradiance_integral(float theta) {
  float sin_theta = std::sin(theta);
  return float(M_PI) * sin_theta * sin_theta;
}

namespace framework {

  // sun size is in radians, not degrees
  void skybox::update(const vec3 & sun_direction_, float sun_size_, const vec3 & ground_albedo_, float turbidity_) {
    vec3 new_sun_direction = sun_direction_;
    new_sun_direction.y = (new_sun_direction.y);
    new_sun_direction = normalize(new_sun_direction);
    float new_sun_size = max(sun_size_, 0.1_degrees);
    float new_turbidity = clamp(turbidity_, 1.f, 32.f);
    vec3 new_ground_albedo = saturate(ground_albedo_);

    if (new_sun_direction == sun_direction && new_sun_size == sun_size && new_ground_albedo == ground_albedo && new_turbidity == turbidity)
      return;

    float theta_sun = angle_between(sun_direction, vec3(0, 1, 0));
    elevation = M_PI_2 - theta_sun;
    sun_direction = new_sun_direction;
    sun_size = new_sun_size;
    turbidity = new_turbidity;
    ground_albedo = new_ground_albedo;

    for (int i = 0;i < 3;++i) {
      if (rgb[i] != nullptr) 
        arhosekskymodelstate_free(rgb[i]);
      rgb[i] = arhosek_rgb_skymodelstate_alloc_init(turbidity, ground_albedo[i], elevation);
    }

    sampled_spectrum ground_albedo_spectrum = sampled_spectrum::from_rgb(albedo);

    ArHosekSkyModelState * sky_states[spectral_samples];
    for (auto i = 0; i < spectral_samples; ++i)
      sky_states[i] = arhosekskymodelstate_alloc_init(theta_sun, turbidity, ground_albedo_spectrum[i]);

    vec3 sun_direction_x = perpendicular(sun_direction);
    mat3 sun_orientation = mat3(sun_direction_x, cross(sun_direction, sun_direction_x), sun_direction);

    const size_t num_samples = 8;
    for (size_t x = 0;x < num_samples; ++x)
      for (size_t y = 0;y < num_samples; ++y) {
        vec3 sample_dir = sun_orientation * sample_direction_cone(
          (x + 0.5f) / num_samples,
          (y + 0.5f) / num_samples,
          cos_physical_sun_size
        );
        float sample_theta_sun = angle_between(sample_dir, vec3(0, 1, 0));
        float sample_gamma = angle_between(sample_dir, sun_direction);

        sampled_spectrum solar_radiance;
        for (size_t i = 0; i < spectral_samples; ++i) {
          float wavelength = lerp(float(sampled_lambda_start), float(sampled_lambda_end), i / float(spectral_samples));
          solar_radiance[i] = float(arhosekskymodel_solar_radiance(sky_states[i], sample_theta_sun, sample_gamma, wavelength));
        }
        vec3 sample_radiance = solar_radiance.to_rgb();

        sun_irradiance += sample_radiance * saturate<float, highp>(dot(sample_dir, sun_direction));
      }

    float pdf = sample_direction_cone_PDF(cos_physical_sun_size);
    sun_irradiance *= (1.0f / num_samples) * (1.0f / num_samples) * (1.0f / pdf);

    // standard luminous efficiency 683 lm/W, coordinate system scaling & scaling to fit into the dynamic range of a 16 bit float
    sun_irradiance *= 683.0f * 100.0f * fp16_scale;
  
    for (auto i = 0; i < spectral_samples; ++i) {
      arhosekskymodelstate_free(sky_states[i]);
      sky_states[i] = nullptr;
    }

    // compute uniform solar radiance value
    sun_radiance = sun_irradiance / irradiance_integral(sun_size);
  }


  skybox::~skybox() {
    for (auto m : rgb) arhosekskymodelstate_free(m);

  }
}