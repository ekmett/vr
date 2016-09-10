#include "stdafx.h"
#include "std.h"
#include "gui.h"
#include "error.h"
#include "openvr_system.h"
#include "timer.h"
#include "quality.h"

namespace framework {
  static int total_dropped_frames = 0;
  static int last_adapted = 0;
  static float old_utilization = 0.8, old_old_utilization = 0.8, utilization = 0.8;
  static vr::Compositor_FrameTiming frame_timing{};

  quality::quality(int quality_level)
  : quality_level(quality_level)
    , sync{ nullptr, nullptr }  {
    create_framebuffers();
  }

  quality::~quality() {
    delete_framebuffers();
    for (auto & t : sync) if (t) glDeleteSync(t);
  }

  void quality::new_frame(openvr::system & vr, float * render_buffer_usage, float * resolve_buffer_usage) {
    if (!suspended_rendering) {
      // adapt quality level
      auto old_frame_index = frame_timing.m_nFrameIndex;
      frame_timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
      bool have_frame_timing = vr::VRCompositor()->GetFrameTiming(&frame_timing, 0);
      if (frame_timing.m_nFrameIndex == old_frame_index)
        log("quality")->warn("same frame");
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

  void quality::submit(int v, int w, int h, bool read_pixel) {
    if (read_pixel) {
      unsigned char pixel[4];
      glBindFramebuffer(GL_READ_FRAMEBUFFER, resolve_target[v].fbo);
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
    for (int i = 0;i < 2;++i) {
      vr::Texture_t eyeTexture{
        (void*)intptr_t(resolve_target[v].texture_view[i]),
        vr::API_OpenGL,
        vr::ColorSpace_Gamma
      };
      vr::VRTextureBounds_t eyeBounds{
        0,
        float(resolve_buffer_h - h) / resolve_buffer_h,
        float(w) / resolve_buffer_w,
        1
      };
      vr::VRCompositor()->Submit(vr::EVREye(i), &eyeTexture, &eyeBounds, vr::Submit_Default);
    }
    vr::VRCompositor()->PostPresentHandoff();
  }

  void quality::swap() {   
    last_viewport_w = viewport_w;
    last_viewport_h = viewport_h;
    if (double_buffer) resolve_index = 1 - resolve_index;
  }

  void quality::present(bool read_pixel) {
    if (suspended_rendering) return;

    if (double_buffer) { 
      int other_index = 1 - resolve_index;
      if (sync[other_index]) {
      retry:
        switch (glClientWaitSync(sync[other_index], GL_SYNC_FLUSH_COMMANDS_BIT, 1000)) { // wait 1 microsecond
          case GL_TIMEOUT_EXPIRED:
            log("quality")->warn("fence timeout");
            goto retry;
          case GL_WAIT_FAILED:
            die("fence sync failed: {}", glewGetErrorString(glGetError()));
            break;
          case GL_CONDITION_SATISFIED: break;
          case GL_ALREADY_SIGNALED: break;
          default:
            die("unknown fence result");
        }
        glDeleteSync(sync[other_index]);
        sync[other_index] = nullptr;
        submit(other_index, last_viewport_w, last_viewport_h, read_pixel);
      }
      sync[resolve_index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    } else {
      submit(resolve_index, viewport_w, viewport_h, read_pixel);
    }
  }

  void quality::delete_framebuffers() {
    for (auto & t : resolve_target) t.finalize();
    for (auto & t : render_target) t.finalize();
  }

  void quality::create_framebuffers() {
    vr::VRSystem()->GetRecommendedRenderTargetSize(&recommended_w, &recommended_h);

    resolve_buffer_w = GLsizei(recommended_w * max_supersampling_factor + 1) & ~1;
    resolve_buffer_h = GLsizei(recommended_h * max_supersampling_factor + 1) & ~1;

    for (int i = 0;i < countof(resolve_target);++i) {
      resolve_target[i].format = { resolve_buffer_w, resolve_buffer_h };
      string name = fmt::format("resolve target {}", i);
      resolve_target[i].initialize(name, GL_RGBA8);
    }

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