#pragma once

#include "spherical_harmonics.h"
#include "glm.h"
#include "shader.h"
#include "uniforms.h"
#include "timer.h"
#include "gui_direction.h"

namespace framework {
  static const float physical_sun_angular_radius = 0.27_degrees;
  static const float fp16_scale = 0.0009765625f; // 2^-10 scaling factor to allow storing physical lights in fp16 floats

  struct sky {
    static const int N = 32; // dimension for cubemap sides

    sky();
    ~sky();

    void update(app_uniforms & uniforms);
    void render() const;

    bool initialized;
    vec3 sun_dir, sun_radiance, sun_irradiance;
    float sun_angular_radius;
    vec3 ground_albedo;
    float turbidity;
    float elevation;

    GLuint cubemap;
    GLuint64 cubemap_handle;
    GLuint cubemap_views[6];
    gl::shader program;
    GLuint vao;
    bool show_skybox_window = true;

    direction_setting direction_editor;
    bool initialized_direction_editor = false;
  };
}
