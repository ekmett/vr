#pragma once

#include "gl.h"
#include "openvr.h"
#include "openvr_system.h"
#include "stereo_fbo.h"

namespace framework {

  static const struct render_target_meta {
    int msaa_level;
    float max_supersampling_factor;
  } render_target_metas[]{
    { 4, 1.1f },
    { 8, 1.4f },
  };

  static const size_t render_target_count = countof(render_target_metas);

  static const struct quality_level {
    int render_target;
    float resolution_scale;
    bool force_interleaved_reprojection;
  } quality_levels[] {
      { 0, 0.81f,  true }
    , { 0, 0.81f, false }
    , { 0, 0.9f,  false }
    , { 0, 1.0f,  false }  // valve 0
    , { 0, 1.1f,  false }
    , { 1, 0.9f,  false }
    , { 1, 1.0f,  false }
    , { 1, 1.1f,  false }
    , { 1, 1.2f,  false }
    , { 1, 1.3f,  false }
    , { 1, 1.4f,  false }    
    //, { 1, 1.5,  false }
    //, { 1, 1.6,  false }
    //, { 1, 1.7,  false }
    //, { 1, 1.8,  false }
  };

  static const float max_supersampling_factor = 1.4f;

  static const size_t quality_level_count = countof(quality_levels);

  struct quality {
    quality(int quality_level = 4);
    ~quality();

    void new_frame(openvr::system & vr, float * render_buffer_usage = nullptr, float * resolve_buffer_usage = nullptr);
    void resolve() { resolve(current_resolve_fbo()); }
    void resolve(stereo_fbo & to);
    void present(bool read_pixel_hack = false); // default to true since the alternative doesn't work
    void submit(int resolve_target, int w, int h, bool read_pixel_hack = false);
    void swap();

  private:
    void delete_framebuffers();
    void create_framebuffers();

  public:
    inline const quality_level & current_quality_level() const { return quality_levels[quality_level]; }
    inline stereo_render_fbo & current_render_fbo() { return render_target[quality_levels[quality_level].render_target]; }
    inline const stereo_render_fbo & current_render_fbo() const { return render_target[quality_levels[quality_level].render_target]; }
    inline stereo_fbo & current_resolve_fbo() {
      return resolve_target[resolve_index];
    }
    inline const stereo_fbo & current_resolve_fbo() const {
      return resolve_target[resolve_index];
    }
    inline stereo_fbo & last_resolve_fbo() {
      return resolve_target[double_buffer ? 1 - resolve_index : resolve_index];
    }
    inline const stereo_fbo & last_resolve_fbo() const {
      return resolve_target[double_buffer ? 1 - resolve_index : resolve_index];
    }

  public:
    float desired_supersampling = 1.0f;

    int minimum_quality_level = 0;
    int maximum_quality_level = quality_level_count - 1;
    int resolve_index = 1;

    bool show_timing_window = true;
    bool show_quality_window = true;
    bool force_interleaved_reprojection = false;    
    bool suspended_rendering = false;

    int quality_level;
    float aspect_ratio, actual_supersampling;

    uint32_t recommended_w, recommended_h;
    GLsizei viewport_w, viewport_h;
    GLsizei last_viewport_w, last_viewport_h;
    GLsizei resolve_buffer_w, resolve_buffer_h;
    float last_resolve_buffer_usage;

    bool using_interleaved_reprojection;

    stereo_render_fbo render_target[render_target_count];
    stereo_fbo resolve_target[2];

    bool double_buffer = false;
    GLsync sync[2];
  };
}