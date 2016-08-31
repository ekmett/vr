#pragma once

#include "framework/spherical_harmonics.h"
#include "framework/glm.h"

struct ArHosekSkyModelState;

namespace framework {


  struct skybox {
    skybox(const vec3 & sun_direction, float sun_size, const vec3 & ground_albedo, float turbidity) {
      update(sun_direction, sun_size, ground_albedo, turbidity);
    }
    ~skybox();
    void update(const vec3 & sun_direction, float sun_size, const vec3 & ground_albedo, float turbidity);
    vec3 sun_direction, sun_radiance, sun_irradiance;
    float sun_size;
    vec3 ground_albedo;
    float turbidity;
    vec3 albedo;
    float elevation = 0.0f;
    sh9_t<vec3> sh;
    ArHosekSkyModelState * rgb[3];
    vec3 sample(const vec3 & dir) const;
    GLuint cubemap;
  };
}