#include "framework/stdafx.h"
#include "framework/std.h"
#include "framework/gui.h"
#include "framework/error.h"
#include "framework/openvr_system.h"
#include "framework/timer.h"
#include "quality.h"

namespace framework {
  static int total_dropped_frames = 0;
  static int last_adapted = 0;
  static float old_utilization = 0.8, old_old_utilization = 0.8, utilization = 0.8;
  static vr::Compositor_FrameTiming frame_timing{};

  quality::quality(int quality_level)
  : quality_level(quality_level) {
    create_framebuffers();
  }

  quality::~quality() {
    delete_framebuffers();
  }

  void quality::new_frame(openvr::system & vr, float * render_buffer_usage, float * resolve_buffer_usage, int * render_target) {
    if (!suspended_rendering) {
      // adapt quality level
      frame_timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
      bool have_frame_timing = vr::VRCompositor()->GetFrameTiming(&frame_timing, 0);
      total_dropped_frames += frame_timing.m_nNumDroppedFrames;

      old_old_utilization = old_utilization;
      old_utilization = utilization;
      bool low_quality = vr::VRCompositor()->ShouldAppRenderWithLowResources();

      utilization = duration<float, std::milli>(frame_timing.m_flClientFrameIntervalMs) / (vr.frame_duration * (low_quality ? 0.75f : 1.f) * (force_interleaved_reprojection ? 2 : 1));
      int quality_change = 0;
      if (last_adapted <= frame_timing.m_nFrameIndex - 2) {
        if (frame_timing.m_nNumDroppedFrames != 0) {
          quality_change = quality_levels[clamp(quality_level - 2, minimum_quality_level, maximum_quality_level)].force_interleaved_reprojection ? -1 : -2;
          last_adapted = frame_timing.m_nFrameIndex;
          log("quality")->info("lowering due to dropped frame");
        } else if (utilization >= 0.9) {
          quality_change = quality_levels[clamp(quality_level - 2, minimum_quality_level, maximum_quality_level)].force_interleaved_reprojection ? -1 : -2;
          last_adapted = frame_timing.m_nFrameIndex;
          log("quality")->info("lowering due to long frame");
        } else if (utilization >= 0.85 && utilization + std::max(utilization - old_utilization, (utilization - old_old_utilization) * 0.5f) >= 0.9) {
          quality_change = quality_levels[clamp(quality_level - 2, minimum_quality_level, maximum_quality_level)].force_interleaved_reprojection ? -1 : -2;
          last_adapted = frame_timing.m_nFrameIndex;
          log("quality")->info("lowering due to predicted long frame");
        } else if (utilization < 0.7 && old_utilization < 0.7 && old_old_utilization < 0.7) { // we have had a good run
          quality_change = +1;
          last_adapted = frame_timing.m_nFrameIndex;
          log("quality")->info("increasing due to short frames");
        }
      }

      quality_level += quality_change;
    }

    quality_level = clamp(quality_level, minimum_quality_level, maximum_quality_level);

    if (show_quality_window) {
      gui::Begin("Quality", &show_quality_window);
      gui::SliderInt("min quality", &minimum_quality_level, 0, quality_level_count - 1);
      gui::SliderInt("quality", &quality_level, 0, quality_level_count - 1);
      gui::SliderInt("max quality", &maximum_quality_level, 0, quality_level_count - 1);
      gui::SliderFloat("super-sampling", &desired_supersampling, 0.3, framework::max_supersampling_factor);
      gui::Checkbox("force interleaved reprojection", &force_interleaved_reprojection);
      gui::End();
    }

    using_interleaved_reprojection = force_interleaved_reprojection || quality_levels[quality_level].force_interleaved_reprojection; //  || (frame_timing.m_nFrameIndex < interleaved_until);
    vr::VRCompositor()->ForceInterleavedReprojectionOn(using_interleaved_reprojection);

    auto q = quality_levels[quality_level];
    actual_supersampling = std::min(q.resolution_scale * desired_supersampling, render_target_metas[q.render_target].max_supersampling_factor);
    
    viewport_w = GLint(recommended_w * actual_supersampling); // &~1; 2x2 blocks, but the presentation window shimmers
    viewport_h = GLint(recommended_h * actual_supersampling); // &~1;
    aspect_ratio = float(viewport_w) / viewport_h;

    if (show_timing_window) {
      gui::Begin("Timing", &show_timing_window);
      gui::Text("viewport: %d x %d (%dx msaa)", viewport_w, viewport_h, render_target_metas[q.render_target].msaa_level);
      gui::Text("frame rate: %2.02f", 1000.0f / frame_timing.m_flClientFrameIntervalMs);
      gui::Text("dropped frames: %d", total_dropped_frames);
      gui::Text("utilization: %.02f", utilization);
      gui::Text("headroom: %.2fms", frame_timing.m_nNumDroppedFrames ? 0.0f : frame_timing.m_flCompositorIdleCpuMs);
      gui::Text("pre-submit GPU: %.2fms", frame_timing.m_flPreSubmitGpuMs);
      gui::Text("post-submit GPU: %.2fms", frame_timing.m_flPostSubmitGpuMs);
      if (using_interleaved_reprojection) gui::Text("Using interleaved reprojection");
      gui::End();
    }

    glEnable(GL_MULTISAMPLE);
    current_render_fbo().bind();

    glStencilMask(1);
    glClearColor(0.18, 0.18, 0.18, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, viewport_w, viewport_h);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // use premultiplied alpha
    glCullFace(GL_BACK);


    if (render_buffer_usage) *render_buffer_usage = actual_supersampling / render_target_metas[q.render_target].max_supersampling_factor;
    if (resolve_buffer_usage) *resolve_buffer_usage = actual_supersampling / max_supersampling_factor;
    if (render_target) *render_target = q.render_target;
  }

  void quality::resolve(stereo_fbo & to) {
    static elapsed_timer timer("resolve msaa");
    timer_block timed(timer);

    auto & from  = current_render_fbo();
    glDisable(GL_MULTISAMPLE);
    for (int i=0;i<2;++i)
      glBlitNamedFramebuffer(
        from.fbo_view[i],
        to.fbo_view[i],
        0, 0, viewport_w, viewport_h,
        0, 0, viewport_w, viewport_h,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
      );
  }

  void quality::present(bool srgb_resolve) {
    if (suspended_rendering) return;
    glFinish();    
    for (int i = 0;i < 2;++i) {
      vr::Texture_t eyeTexture{
        (void*)intptr_t(resolve_target.texture_view[i]),
        vr::API_OpenGL,
        srgb_resolve ? vr::ColorSpace_Gamma : vr::ColorSpace_Linear
      };
      vr::VRTextureBounds_t eyeBounds{
        0,
        float(resolve_buffer_h - viewport_h) / resolve_buffer_h,
        float(viewport_w) / resolve_buffer_w,
        1
      };
      vr::VRCompositor()->Submit(vr::EVREye(i), &eyeTexture, &eyeBounds, vr::Submit_Default);
    }
    //compositor.PostPresentHandoff();
  }

  void quality::delete_framebuffers() {
    resolve_target.finalize();
    for (auto & t : render_target) t.finalize();
  }

  void quality::create_framebuffers() {
    vr::VRSystem()->GetRecommendedRenderTargetSize(&recommended_w, &recommended_h);

    resolve_buffer_w = GLsizei(recommended_w * max_supersampling_factor + 1) & ~1;
    resolve_buffer_h = GLsizei(recommended_h * max_supersampling_factor + 1) & ~1;

    resolve_target.format = { resolve_buffer_w, resolve_buffer_h };
    resolve_target.initialize("resolve", GL_RGBA8);

    for (int i = 0;i < render_target_count;++i) {
      auto meta = render_target_metas[i];
      render_target[i].format = {
        GLsizei(recommended_w * meta.max_supersampling_factor + 1) & ~1,
        GLsizei(recommended_h * meta.max_supersampling_factor + 1) & ~1,
        meta.msaa_level,
        false,
        GL_DEPTH24_STENCIL8
      };
      string name = fmt::format("render target {} ({}x msaa)", i, meta.msaa_level);
      render_target[i].initialize(name, GL_RGBA16F);
    }
  }
}