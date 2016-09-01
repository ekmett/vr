#pragma once

#include "framework/spherical_harmonics.h"
#include "framework/glm.h"
#include "framework/shader.h"

struct ArHosekSkyModelState;

namespace framework {
  static const float physical_sun_size = 0.27_degrees;
  static const float fp16_scale = 0.0009765625f; // 2^-10 scaling factor to allow storing physical lights in fp16 floats

  struct sky {
    sky(const vec3 & sun_direction, float sun_size, const vec3 & ground_albedo, float turbidity);
    ~sky();

    void update(const vec3 & sun_direction, float sun_size, const vec3 & ground_albedo, float turbidity);
    vec3 sample(const vec3 & dir) const;
    void render(const mat4 perspective[2], const mat4 model_view[2]) const;

    vec3 sun_direction, sun_radiance, sun_irradiance;
    float sun_size;
    vec3 ground_albedo;
    float turbidity;
    float elevation;
    sh9_t<vec3> sh;
    ArHosekSkyModelState * rgb[3];
    GLuint testmap; // for showing
    GLuint cubemap;
    GLuint64 cubemap_handle;
    gl::shader program;
    GLuint ubo;
    GLuint vao;
  };
}