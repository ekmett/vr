#include "framework/stdafx.h"
#include "framework/std.h"
#include "framework/gui.h"
#include "framework/error.h"
#include "framework/openvr_system.h"
#include "quality.h"

namespace framework {
  static int total_dropped_frames = 0;
  static int last_adapted = 0;
  static float old_utilization = 0.8, old_old_utilization = 0.8, utilization = 0.8;
  static vr::Compositor_FrameTiming frame_timing{};

  quality::quality(int quality_level) : quality_level(quality_level) {
    create_framebuffers();
  }

  quality::~quality() {
    delete_framebuffers();
  }

  void quality::new_frame(openvr::system & vr, float * render_buffer_usage, float * resolve_buffer_usage) {

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
      gui::Text("viewport: %d x %d (%dx msaa) (%.02fx supersampling)", viewport_w, viewport_h, render_target_metas[q.render_target].msaa_level, actual_supersampling);
      gui::Text("frame rate: %2.02f", 1000.0f / frame_timing.m_flClientFrameIntervalMs);
      gui::Text("dropped frames: %d", total_dropped_frames);
      gui::Text("utilization: %.02f", utilization);
      gui::Text("headroom: %.2fms", frame_timing.m_nNumDroppedFrames ? 0.0f : frame_timing.m_flCompositorIdleCpuMs);
      //gui::Text("time waiting for present: %.2fms", frame_timing.m_flWaitForPresentCpuMs);
      gui::Text("pre-submit GPU: %.2fms", frame_timing.m_flPreSubmitGpuMs);
      gui::Text("post-submit GPU: %.2fms", frame_timing.m_flPostSubmitGpuMs);
      // gui::Text("frame: %d", frame_timing.m_nFrameIndex);
      // gui::Text("client frame interval %.2fms", frame_timing.m_flClientFrameIntervalMs);
      // gui::Text("total render GPU: %.2fms", frame_timing.m_flTotalRenderGpuMs);
      // gui::Text("compositor render GPU: %.2fms", frame_timing.m_flCompositorRenderGpuMs);
      // gui::Text("compositor render CPU: %.2fms", frame_timing.m_flCompositorRenderCpuMs);
      if (using_interleaved_reprojection) gui::Text("Using interleaved reprojection");
      gui::End();
    }


    glEnable(GL_MULTISAMPLE);
    glBindFramebuffer(GL_FRAMEBUFFER, render_fbo[q.render_target]);
    glStencilMask(1);
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f); // clearing a layered framebuffer clears all layers.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, viewport_w, viewport_h);

    if (render_buffer_usage) *render_buffer_usage = actual_supersampling / render_target_metas[q.render_target].max_supersampling_factor;
    if (resolve_buffer_usage) *resolve_buffer_usage = actual_supersampling / max_supersampling_factor;
  }

  void quality::present() {
    auto q = quality_levels[quality_level];
    glDisable(GL_MULTISAMPLE);
    glBlitNamedFramebuffer(
      render_fbo[q.render_target],
      resolve_fbo,
      0, 0, viewport_w, viewport_h,
      0, 0, viewport_w, viewport_h,
      GL_COLOR_BUFFER_BIT,
      GL_LINEAR
    );
    glBlitNamedFramebuffer(
      render_view_fbo[q.render_target],
      resolve_view_fbo,
      0, 0, viewport_w, viewport_h,
      0, 0, viewport_w, viewport_h,
      GL_COLOR_BUFFER_BIT,
      GL_LINEAR
    );
    if (suspended_rendering) return;
    //::Sleep(4);
    glFinish(); // drastic -- start using only when we start dropping frames?
    for (int i = 0;i < 2;++i) {
      vr::Texture_t eyeTexture{
        (void*)intptr_t(resolve_view_texture[i]),
        vr::API_OpenGL,
        vr::ColorSpace_Gamma
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
    glDeleteFramebuffers(countof(fbo), fbo);
    glDeleteTextures(countof(texture), texture);
  }

  void quality::create_framebuffers() {
    vr::VRSystem()->GetRecommendedRenderTargetSize(&recommended_w, &recommended_h);
    glCreateFramebuffers(countof(fbo), fbo);
    glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, countof(render_texture), render_texture);
    glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, countof(render_depth_stencil), render_depth_stencil);
    glGenTextures(countof(render_view_texture), render_view_texture);

    // generate render target fbos
    for (int i = 0;i < render_target_count; ++i) {
      auto meta = render_target_metas[i];
      gl::label(GL_FRAMEBUFFER, render_fbo[i], "render fbo {} ({}x msaa)", i, meta.msaa_level);
      gl::label(GL_TEXTURE, render_texture[i], "render texture array {} ({}x msaa)", i, meta.msaa_level);
      gl::label(GL_TEXTURE, render_depth_stencil[i], "render depth/stencil array {} ({}x msaa)", i, meta.msaa_level);
      glTextureStorage3DMultisample(
        render_texture[i],
        meta.msaa_level,
        GL_RGBA16F,
        recommended_w * meta.max_supersampling_factor,
        recommended_h * meta.max_supersampling_factor,
        2,
        true
      );

      glTextureParameteri(render_texture[i], GL_TEXTURE_MAX_LEVEL, 0);
      glNamedFramebufferTexture(render_fbo[i], GL_COLOR_ATTACHMENT0, render_texture[i], 0);

      glTextureStorage3DMultisample(
        render_depth_stencil[i],
        meta.msaa_level,
        GL_DEPTH32F_STENCIL8,
        recommended_w * meta.max_supersampling_factor,
        recommended_h * meta.max_supersampling_factor,
        2,
        true
      );

      glTextureParameteri(render_depth_stencil[i], GL_TEXTURE_MAX_LEVEL, 0);
      glNamedFramebufferTexture(render_fbo[i], GL_DEPTH_STENCIL_ATTACHMENT, render_depth_stencil[i], 0);

      gl::check_framebuffer(render_fbo[i], GL_FRAMEBUFFER);

      // Needed to glBlitFrameBuffer the right hand eye, but I don't blit the depth buffer, so I don't need a copy of that
      gl::label(GL_FRAMEBUFFER, render_view_fbo[i], "render view fbo {}", i);
      glTextureView(render_view_texture[i], GL_TEXTURE_2D_MULTISAMPLE, render_texture[i], GL_RGBA16F, 0, 1, 1, 1);
      gl::label(GL_TEXTURE, render_view_texture[i], "render view texture {}", i);
      glTextureParameteri(render_view_texture[i], GL_TEXTURE_MAX_LEVEL, 0);
      glNamedFramebufferTexture(render_view_fbo[i], GL_COLOR_ATTACHMENT0, render_view_texture[i], 0);

      gl::check_framebuffer(render_view_fbo[i], GL_FRAMEBUFFER);
    }

    gl::label(GL_FRAMEBUFFER, resolve_fbo, "resolve fbo");
    gl::label(GL_FRAMEBUFFER, resolve_view_fbo, "resolve view fbo");

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &resolve_texture);
    gl::label(GL_TEXTURE, resolve_texture, "resolve texture");
    resolve_buffer_w = GLsizei(recommended_w * max_supersampling_factor) & ~1;
    resolve_buffer_h = GLsizei(recommended_h * max_supersampling_factor) & ~1;
    glTextureStorage3D(resolve_texture, 1, GL_RGBA8, resolve_buffer_w, resolve_buffer_h, 2);
    glTextureParameteri(resolve_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(resolve_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(resolve_texture, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(resolve_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resolve_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(resolve_fbo, GL_COLOR_ATTACHMENT0, resolve_texture, 0);

    gl::check_framebuffer(resolve_fbo, GL_FRAMEBUFFER);

    glGenTextures(countof(resolve_view_texture), resolve_view_texture);
    for (int i = 0;i < 2;++i) {
      glTextureView(resolve_view_texture[i], GL_TEXTURE_2D, resolve_texture, GL_RGBA8, 0, 1, i, 1);
      glTextureParameteri(resolve_view_texture[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTextureParameteri(resolve_view_texture[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(resolve_view_texture[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(resolve_view_texture[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glNamedFramebufferTexture(resolve_view_fbo, GL_COLOR_ATTACHMENT0, resolve_view_texture[1], 0); // bind the right hand resolve texture to our 'view' fbo for blitting

    gl::check_framebuffer(resolve_view_fbo, GL_FRAMEBUFFER);
  }
}