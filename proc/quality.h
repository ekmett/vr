#pragma once

#include "framework/gl.h"
#include "framework/openvr.h"

namespace framework {

  static const struct render_target_meta {
    int msaa_level;
    float max_supersampling_factor;
  } render_target_metas[]{
    { 4, 1.1 },
    { 8, 1.4 }
  };

  static const int render_target_count = countof(render_target_metas);

  static const struct quality_level {
    int render_target;
    float resolution_scale;
    bool force_interleaved_reprojection;
  } quality_levels[] = {
    { 0, 0.81,  true },
    { 0, 0.81, false },
    { 0, 0.9,  false },
    { 0, 1.0,  false },  // valve 0
    { 0, 1.1,  false },
    { 1, 0.9,  false },
    { 1, 1.0,  false },
    { 1, 1.1,  false },
    { 1, 1.2,  false },
    { 1, 1.3,  false },
    { 1, 1.4,  false }
  };

  static const float max_supersampling_factor = 1.4;

  static const int quality_level_count = countof(quality_levels);

  struct quality {
    quality(int quality_level);
    ~quality();

    void new_frame(openvr::system & vr, float * render_buffer_usage = nullptr, float * resolve_buffer_usage = nullptr);
    void present();

  private:
    void delete_framebuffers();
    void create_framebuffers();
    inline const quality_level & current_quality_level() const { return quality_levels[quality_level]; }
    inline GLuint current_render_fbo() const { return render_fbo[quality_levels[quality_level].render_target]; }

  public:
    float desired_supersampling = 1.0f;
    int minimum_quality_level = 0;
    int maximum_quality_level = quality_level_count - 1;
    bool show_timing_window = true;
    bool show_quality_window = true;
    bool force_interleaved_reprojection = false;    
    bool suspended_rendering = false;

    int quality_level;
    float aspect_ratio, actual_supersampling;
    uint32_t recommended_w, recommended_h;
    GLsizei viewport_w, viewport_h;
    GLsizei resolve_buffer_w, resolve_buffer_h;
    bool using_interleaved_reprojection;

    union {
      GLuint fbo[render_target_count * 2 + 2];
      struct {
        GLuint render_fbo[render_target_count], render_view_fbo[render_target_count], resolve_fbo, resolve_view_fbo;
      };
    };
    union {
      GLuint texture[render_target_count * 3 + 3];
      struct {
        GLuint render_texture[render_target_count],
          render_depth_stencil[render_target_count],
          render_view_texture[render_target_count],
          resolve_texture,
          resolve_view_texture[2];
      };
    };
  };
}