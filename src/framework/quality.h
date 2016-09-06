#pragma once

#include "framework/gl.h"
#include "framework/openvr.h"
#include "framework/stereo_fbo.h"

namespace framework {

  static const struct render_target_meta {
    int msaa_level;
    float max_supersampling_factor;
  } render_target_metas[]{
    { 4, 1.1f },
    { 8, 1.4f }
  };

  static const int render_target_count = countof(render_target_metas);

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

  static const float max_supersampling_factor = 1.4;

  static const int quality_level_count = countof(quality_levels);

  struct quality {
    quality(int quality_level = 4);
    ~quality();

    void new_frame(openvr::system & vr, float * render_buffer_usage = nullptr, float * resolve_buffer_usage = nullptr, int * render_target = nullptr);
    void resolve() { resolve(resolve_target); }
    void resolve(stereo_fbo & to);
    void present(bool srgb_resolve = true); // default to true since the alternative doesn't work

  private:
    void delete_framebuffers();
    void create_framebuffers();
    inline const quality_level & current_quality_level() const { return quality_levels[quality_level]; }

    inline stereo_render_fbo & current_render_fbo() { return render_target[quality_levels[quality_level].render_target]; }
    inline const stereo_render_fbo & current_render_fbo() const { return render_target[quality_levels[quality_level].render_target]; }

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

    stereo_render_fbo render_target[render_target_count];
    stereo_fbo resolve_target;
  };
}