#include "skybox.h"
#include "framework/glm.h"
#include <cmath>
#include <algorithm>
#include "framework/spectrum.h"

static inline float angle_between(const glm::vec3 & x, const glm::vec3 & y) {
  return std::acosf(std::max(glm::dot(x,y), 0.00001f));
}

namespace framework {

  void skybox::update(const vec3 & sun_direction_, float sun_size_, const vec3 & ground_albedo_, float turbidity_) {
    vec3 new_sun_direction = sun_direction_;
    new_sun_direction.y = (new_sun_direction.y);
    new_sun_direction = normalize(new_sun_direction);
    float new_sun_size = max(sun_size_, 0.1f);
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

    sampled_spectrum ground_albedo_spectrum = sampled_spectrum::from_rgb(albedo);
    sampled_spectrum solar_radiance;

    ArHosekSkyModelState * sky_states[spectral_samples];
    for (auto i = 0; i < spectral_samples; ++i)
      sky_states[i] = arhosekskymodelstate_alloc_init(theta_sun, ground_albedo_spectrum[i]);

    // ...

    for (auto i = 0; i < spectral_samples; ++i)
      arhosekskymodelstate_free(sky_states[i]);

    sun_irradiance = vec3(0);
  }


  skybox::~skybox() {

  }
}