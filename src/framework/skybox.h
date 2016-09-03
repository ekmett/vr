#pragma once

#include "framework/spherical_harmonics.h"
#include "framework/glm.h"
#include "framework/shader.h"
#include "uniforms.h" 

struct ArHosekSkyModelState;

namespace framework {
  static const float physical_sun_size = 0.27_degrees;
  static const float fp16_scale = 0.0009765625f; // 2^-10 scaling factor to allow storing physical lights in fp16 floats

  struct sky {
    sky(const vec3 & sun_direction, float sun_size, const vec3 & ground_albedo, float turbidity, app_uniforms & uniforms);
    ~sky();

    void update(const vec3 & sun_direction, float sun_size, const vec3 & ground_albedo, float turbidity, app_uniforms & uniforms);
    vec3 sample(const vec3 & dir) const;
    void render() const;

    vec3 sun_direction, sun_radiance, sun_irradiance;
    float sun_size;
    vec3 ground_albedo;
    float turbidity;
    float elevation;
    sh9_t<vec3> sh;
    ArHosekSkyModelState * rgb[3];
    GLuint cubemap;
    GLuint64 cubemap_handle;
    GLuint cubemap_views[6];
    gl::shader program;
  };
}