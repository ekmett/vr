#pragma once

#include "framework/spherical_harmonics.h"
#include "framework/glm.h"
#include "framework/shader.h"
#include "uniforms.h" 

struct ArHosekSkyModelState;

namespace framework {
  static const float physical_sun_angular_radius = 0.27_degrees;
  static const float fp16_scale = 0.0009765625f; // 2^-10 scaling factor to allow storing physical lights in fp16 floats

  struct sky {
    static const int N = 128; // dimension for cubemap sides

    sky();
    ~sky();

    void update(app_uniforms & uniforms);
    vec3 sample(const vec3 & dir) const;
    void render() const;

    bool initialized;
    vec3 sun_dir, sun_radiance, sun_irradiance;
    float sun_angular_radius;
    vec3 ground_albedo;
    float turbidity;
    float elevation;

    sh9_t<vec3> sh;
    ArHosekSkyModelState * rgb[3];
    GLuint cubemap;
    GLuint64 cubemap_handle;
    GLuint cubemap_views[6];
    gl::shader program;
    GLuint vao;

    bool show_skybox_window = true;
  };
}
