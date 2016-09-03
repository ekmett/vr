#include "stdafx.h"
#include <random>
#include <functional>
#include <algorithm>
#include "framework/worker.h"
#include "framework/gl.h"
#include "framework/signal.h"
#include "framework/filesystem.h"
#include "controllers.h"
#include "framework/gui.h"
#include "imgui_internal.h"
#include <glm/gtc/matrix_transform.hpp>
#include "framework/openal.h"
#include "distortion.h"
#include "overlay.h"
#include "framework/skybox.h"
#include "framework/spectrum.h"
#include "uniforms.h"

using namespace framework;
using namespace filesystem;

static const char * show_framebuffer_status_result(GLenum e) {
  switch (e) {
    case GL_FRAMEBUFFER_COMPLETE: return "complete";
    case GL_FRAMEBUFFER_UNDEFINED: return "undefined";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "incomplete attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "incomplete missing attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "incomplete draw buffer";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "incomplete read buffer";
    case GL_FRAMEBUFFER_UNSUPPORTED: return "unsupported";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "incomplete multisample";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "incomplete layer targets";
    default: return "unknown";
  }
}

void check_framebuffer(GLuint framebuffer, GLenum role) {
  GLenum result = glCheckNamedFramebufferStatus(framebuffer, role);
  if (result != GL_FRAMEBUFFER_COMPLETE) {
    GLsizei len;
    glGetObjectLabel(GL_FRAMEBUFFER, framebuffer, 0, &len, nullptr);
    string label;
    label.resize(len + 1);
    GLsizei final_len;
    glGetObjectLabel(GL_FRAMEBUFFER, framebuffer, len, &final_len, const_cast<GLchar*>(label.c_str()));
    label.resize(len);
    die("bad framebuffer status: {} ({}): {}", label, framebuffer, show_framebuffer_status_result(result));
  }
}

// valve-style adaptive rendering quality

static const struct render_target_meta {
  int msaa_level;
  float max_supersampling_factor;
} render_target_metas[] {
  { 2, 1.5 },
  { 4, 1.5 }
  //{ 8, 1.4 }
};

static const int render_target_count = countof(render_target_metas);

static const struct quality_level {
  int render_target;
  float resolution_scale;
  bool force_interleaved_reprojection;
} quality_levels[] = {
  { 0, 0.65, true },   
  { 0, 0.81, true },   
  { 0, 0.81, false },
  { 0, 1.0,  false },
  { 0, 1.1,  false },
  { 0, 1.4,  false },
  { 1, 0.9,  false },  
  { 1, 1.0,  false },  // valve 0
  { 1, 1.1,  false },
  { 1, 1.22,  false }, // largest 4x supersampling factor = 1.4
  { 1, 1.4,   false }
  //{ 2, 0.9,   false },
  //{ 2, 1.0,   false },
  //{ 2, 1.1,   false },
  //{ 2, 1.22,  false }
};

static const float max_supersampling_factor = 1.5;


static const int quality_level_count = countof(quality_levels);

struct viewport_dim {
  GLint x, y, z, w, h;
};

// used reversed [1..0] floating point z rather than the classic [-1..1] mapping
#define USE_REVERSED_Z

struct app : app_uniforms {
  app();
  ~app();

  void run();
  bool show_gui(bool * open = nullptr);
  void get_poses();
  void adapt_quality();
  void tonemap();
  void present();
  void update_controller_assignment();
  void submit_uniforms() {
    global_time = SDL_GetTicks() / 1000.0f;
    glNamedBufferSubData(ubo, 0, sizeof(app_uniforms), static_cast<app_uniforms*>(this));
  }
 
  sdl::window window; // must come before anything that needs opengl support in this object
  gl::compiler compiler;
  GLuint ubo;
  GLuint dummy_vao;
  openvr::system vr;
  framework::sky sky;

  bool debug_wireframe_hidden_area = false;
  bool debug_wireframe_distortion = false;
  bool show_settings_window = true;
  bool show_demo_window = false;
  bool show_timing_window = true;
  bool suspended_rendering = false;
  float desired_supersampling = 1.0f;
  GLint viewport_w, viewport_h;
  
  gui::system gui;
  controllers controllers;
  std::mt19937 rng; // for the main thread  
  vr::IVRCompositor & compositor;
  distortion distorted;
  openal::system al;
  int desktop_view;
  bool skybox_visible = true;
  int controller_mask;
  int quality_tick;
  uint32_t recommended_w, recommended_h;
  uint32_t resolve_buffer_w, resolve_buffer_h;
  bool force_interleaved_reprojection;

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

  viewport_dim viewports[2];
  
  inline const ::quality_level & current_quality_level() const {
    return quality_levels[quality_level];
  }
  inline GLuint current_render_fbo() const {
    return render_fbo[quality_levels[quality_level].render_target];
  }
};

#ifdef USE_REVERSED_Z
static float reverseZ_contents[16] = {
   1.f, 0.f, 0.f, 0.f,
   0.f, 1.f, 0.f, 0.f,
   0.f, 0.f, -1.f, 1.f,
   0.f, 0.f, 1.f, 0.f 
};

static mat4 reverseZ = glm::make_mat4(reverseZ_contents);
#endif

app::app()
  : window("proc", { 4, 5, gl::profile::core }, true, 50, 50, 1280, 1024)
  , compiler(path("shaders"))
  , vr()
  , compositor(*vr::VRCompositor())
  , gui(window)
  , distorted()
  , desktop_view(1)
  //, sky(vec3(0.0, 0.1, 0.8), 2.0_degrees, vec3(0.2, 0.2, 0.4), 6.f, *this)
  , sky(vec3(0.252, 0.955, -.155), 2.0_degrees, vec3(0.25, 0.25, 0.25), 2.f, *this)
  , debug_wireframe_hidden_area(false)
  {

  nearClip = 0.1f;
  farClip = 10000.f;
  minimum_quality_level = 0;
  quality_level = 6;
  maximum_quality_level = quality_level_count - 1;
  force_interleaved_reprojection = false;
  enable_seascape = false;

  glCreateVertexArrays(1, &dummy_vao); // we'll load this as needed
  glCreateBuffers(1, &ubo);
  gl::label(GL_BUFFER, ubo, "app ubo");
  glNamedBufferStorage(ubo, sizeof(app_uniforms), nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo); // INVARIANT: we keep this in this slot forever

  // load matrices.
  for (int i = 0;i < 2;++i) {
#ifdef USE_REVERSED_Z
    projection[i] = reverseZ * openvr::hmd_mat4(vr.handle->GetProjectionMatrix(vr::EVREye(i), nearClip, farClip, vr::API_DirectX));
#else
    projection[i] = openvr::hmd_mat4(vr.handle->GetProjectionMatrix(vr::EVREye(i), nearClip, farClip, vr::API_OpenGL));
#endif
    inverse_projection[i] = inverse(projection[i]);
    eye_to_head[i] = openvr::hmd_mat3x4(vr.handle->GetEyeToHeadTransform(vr::EVREye(i)));
    head_to_eye[i] = affineInverse(eye_to_head[i]);
  }

#ifdef USE_REVERSED_Z
  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
  glDepthFunc(GL_GREATER);
  glClearDepth(0.f);
#endif

  vr.handle->GetRecommendedRenderTargetSize(&recommended_w, &recommended_h);
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

    check_framebuffer(render_fbo[i], GL_FRAMEBUFFER);

    // Needed to glBlitFrameBuffer the right hand eye, but I don't blit the depth buffer, so I don't need a copy of that
    gl::label(GL_FRAMEBUFFER, render_view_fbo[i], "render view fbo {}", i);    
    glTextureView(render_view_texture[i], GL_TEXTURE_2D_MULTISAMPLE, render_texture[i], GL_RGBA16F, 0, 1, 1, 1);
    gl::label(GL_TEXTURE, render_view_texture[i], "render view texture {}", i);
    glTextureParameteri(render_view_texture[i], GL_TEXTURE_MAX_LEVEL, 0);
    glNamedFramebufferTexture(render_view_fbo[i], GL_COLOR_ATTACHMENT0, render_view_texture[i], 0);

    check_framebuffer(render_view_fbo[i], GL_FRAMEBUFFER);
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

  check_framebuffer(resolve_fbo, GL_FRAMEBUFFER);

  log("app")->info("creating resolve views");
  glGenTextures(countof(resolve_view_texture), resolve_view_texture);
  for (int i = 0;i < 2;++i) {
    glTextureView(resolve_view_texture[i], GL_TEXTURE_2D, resolve_texture, GL_RGBA8, 0, 1, i, 1);
    glTextureParameteri(resolve_view_texture[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(resolve_view_texture[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(resolve_view_texture[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(resolve_view_texture[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  glNamedFramebufferTexture(resolve_view_fbo, GL_COLOR_ATTACHMENT0, resolve_view_texture[1], 0); // bind the right hand resolve texture to our 'view' fbo for blitting

  check_framebuffer(resolve_view_fbo, GL_FRAMEBUFFER);

  SDL_DisplayMode wdm;
  SDL_GetWindowDisplayMode(window.sdl_window, &wdm);
  log("app")->info("desktop window refresh rate: {}hz", wdm.refresh_rate); // TODO: compute frame skipping for gui and desktop display off this relative to the gpu rate.

  SDL_StartTextInput();
  submit_uniforms(); // load up some data
}

app::~app() {
  SDL_StopTextInput();
  glDeleteVertexArrays(1, &dummy_vao);
  glDeleteFramebuffers(countof(fbo), fbo);
  glDeleteTextures(countof(texture), texture);
}

viewport_dim fit_viewport(float aspectRatio, int w, int h, bool bottom_justified = true, bool left_justified = false) {
  GLint x = GLint(h * aspectRatio);
  if (x <= w) {
    return viewport_dim { left_justified ? 0 : (w-x)/2, 0, 0, x, h };
  } else {
    GLint y = GLint(w / aspectRatio);
    return viewport_dim { 0, bottom_justified ? 0 : (h-y)/2, 0, w, y };
  }
}

void app::update_controller_assignment() {
  if (controller_mask == 3) return;
  auto hmd = vr::VRSystem();
  controller_device[0] = hmd->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
  controller_device[1] = hmd->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
  controller_mask = 0;
  for (int i = 0;i < 2;++i) {
    int k = controller_device[i];
    if (k == vr::k_unTrackedDeviceIndexInvalid) continue;
    if (!hmd->IsTrackedDeviceConnected(k)) continue;
    if (hmd->GetTrackedDeviceClass(k) != vr::TrackedDeviceClass_Controller) continue; // sanity check
    controller_mask |= (1 << i);
  }
}

void app::tonemap() {
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
}

void app::present() {
  if (suspended_rendering) return;
  for (int i = 0;i < 2;++i) {
    vr::Texture_t eyeTexture { 
     (void*)intptr_t(resolve_view_texture[i]), 
     vr::API_OpenGL, 
     vr::ColorSpace_Gamma 
    };
    vr::VRTextureBounds_t eyeBounds {
      0,
      float(resolve_buffer_h - viewport_h) / resolve_buffer_h,
      float(viewport_w) / resolve_buffer_w,
      1
    };
    vr::VRCompositor()->Submit(vr::EVREye(i), &eyeTexture, &eyeBounds, vr::Submit_Default);
  }
  compositor.PostPresentHandoff();
}

void app::get_poses() {
  vr::TrackedDevicePose_t physical_pose[vr::k_unMaxTrackedDeviceCount]; // current poses
  vr::TrackedDevicePose_t predicted_pose[vr::k_unMaxTrackedDeviceCount]; // poses 2 frames out

  compositor.WaitGetPoses(physical_pose, vr::k_unMaxTrackedDeviceCount, predicted_pose, vr::k_unMaxTrackedDeviceCount);

  for (int i = 0;i < vr::k_unMaxTrackedDeviceCount; ++i) {
    current_device_to_world[i] = openvr::hmd_mat3x4(physical_pose[i].mDeviceToAbsoluteTracking);
    predicted_device_to_world[i] = openvr::hmd_mat3x4(predicted_pose[i].mDeviceToAbsoluteTracking);
  }

  update_controller_assignment();

  for (int i = 0;i < 2;++i) {
    if (controller_mask & (1 << i)) {
      current_controller_to_world[i] = current_device_to_world[controller_device[i]];
      predicted_controller_to_world[i] = predicted_device_to_world[controller_device[i]];
    } else {
      log("app")->warn("Unable to update controller position {}", i);
    }
  }

  current_world_to_head = affineInverse(current_device_to_world[vr::k_unTrackedDeviceIndex_Hmd]);
  predicted_world_to_head = affineInverse(predicted_device_to_world[vr::k_unTrackedDeviceIndex_Hmd]);

  for (int i = 0; i < 2; ++i) {
    current_pmv[i] = projection[i] * head_to_eye[i] * current_world_to_head;
    predicted_pmv[i] = projection[i] * head_to_eye[i] * predicted_world_to_head;
  }
}

void app::adapt_quality() {
  int total_dropped_frames = 0;
  int last_adapted = 0;
  static float old_utilization = 0.8, old_old_utilization = 0.8, utilization = 0.8;

  // adapt quality level
  vr::Compositor_FrameTiming frame_timing{};
  frame_timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
  bool have_frame_timing = vr::VRCompositor()->GetFrameTiming(&frame_timing, 0);

  old_old_utilization = old_utilization;
  old_utilization = utilization;
  bool low_quality = vr::VRCompositor()->ShouldAppRenderWithLowResources();

  utilization = duration<float, std::milli>(frame_timing.m_flClientFrameIntervalMs) / (vr.frame_duration * (low_quality ? 0.75f : 1.f) * (force_interleaved_reprojection ? 2 : 1));
  if (frame_timing.m_nNumDroppedFrames != 0) {
    utilization = std::max(2.0f, utilization);
  }
  int quality_change = 0;
  if (last_adapted < frame_timing.m_nFrameIndex - 2) {
    if (frame_timing.m_nNumDroppedFrames != 0) {
      quality_change = quality_levels[clamp(quality_level - 2, minimum_quality_level, maximum_quality_level)].force_interleaved_reprojection ? -1 : -2;
      last_adapted = frame_timing.m_nFrameIndex;
      log("app")->warn("lowering quality due to dropped frame");
      // interleaved_until = frame_timing.m_nFrameIndex + 10; // try to avoid hiccuping on the screen? * log(total_dropped_frames);
    } else if (utilization >= 0.9) {
      quality_change = quality_levels[clamp(quality_level - 2, minimum_quality_level, maximum_quality_level)].force_interleaved_reprojection ? -1 : -2;
      last_adapted = frame_timing.m_nFrameIndex;
      log("app")->info("lowering quality due to long frame");
    } else if (utilization >= 0.85 && utilization + std::max(utilization - old_utilization, (utilization - old_old_utilization) * 0.5f) >= 0.9) {
      quality_change = quality_levels[clamp(quality_level - 2, minimum_quality_level, maximum_quality_level)].force_interleaved_reprojection ? -1 : -2;
      last_adapted = frame_timing.m_nFrameIndex;
      log("app")->info("lowering quality due to predicted long frame");
    } else if (utilization < 0.7 && old_utilization < 0.7 && old_old_utilization < 0.7) { // we have had a good run
      quality_change = +1;
      last_adapted = frame_timing.m_nFrameIndex;
      log("app")->info("increasing quality due to short frames");
    }
  }

  quality_level += quality_change;

  quality_level = clamp(quality_level, minimum_quality_level, maximum_quality_level);
  bool interleaved_reprojection = force_interleaved_reprojection || quality_levels[quality_level].force_interleaved_reprojection; //  || (frame_timing.m_nFrameIndex < interleaved_until);
  vr::VRCompositor()->ForceInterleavedReprojectionOn(interleaved_reprojection);

  if (gui::Begin("Timing", &show_timing_window)) {
    gui::Text("dropped frames: %d", total_dropped_frames);
    gui::Text("utilization: %.02f", utilization);
    gui::Text("headroom: %.2fms", frame_timing.m_nNumDroppedFrames ? 0.0f : frame_timing.m_flCompositorIdleCpuMs);
    gui::Text("time waiting for present: %.2fms", frame_timing.m_flWaitForPresentCpuMs);
    gui::Text("pre-submit GPU: %.2fms", frame_timing.m_flPreSubmitGpuMs);
    gui::Text("post-submit GPU: %.2fms", frame_timing.m_flPostSubmitGpuMs);
    // gui::Text("frame: %d", frame_timing.m_nFrameIndex);
    // gui::Text("client frame interval %.2fms", frame_timing.m_flClientFrameIntervalMs);
    // gui::Text("total render GPU: %.2fms", frame_timing.m_flTotalRenderGpuMs);
    // gui::Text("compositor render GPU: %.2fms", frame_timing.m_flCompositorRenderGpuMs);
    // gui::Text("compositor render CPU: %.2fms", frame_timing.m_flCompositorRenderCpuMs);
    if (interleaved_reprojection) gui::Text("Using interleaved reprojection");
    gui::End();
  }
}

void app::run() { 
  while (!vr.poll() && !window.poll()) {
    // start a new imgui frame before we think of doing anything else
    log("frame")->info("updating imgui");
    gui.new_frame();
    log("frame")->info("adapting quality");
    adapt_quality();
    log("frame")->info("getting poses");
    get_poses();

    log("frame")->info("clearing screen");

    // clear the display window  
    glClearColor(0.18f, 0.18f, 0.18f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    log("frame")->info("clearing render");

    // hmd
    glEnable(GL_MULTISAMPLE);
    auto q = quality_levels[quality_level];
    glBindFramebuffer(GL_FRAMEBUFFER, render_fbo[q.render_target]);
    glStencilMask(1);
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f); // clearing a layered framebuffer clears all layers.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    float actual_supersampling = std::min(q.resolution_scale * desired_supersampling, render_target_metas[q.render_target].max_supersampling_factor);

    // even numbers so we don't end a 2x2 block in the middle.
    viewport_w = GLint(recommended_w * actual_supersampling) & ~1;
    viewport_h = GLint(recommended_h * actual_supersampling) & ~1;

    log("frame")->info("establishing viewport: {} x {}", viewport_w, viewport_h);

    gui::Text("viewport: %d x %d (%.02fx supersampling)", viewport_w, viewport_h, actual_supersampling);

    float aspect_ratio = float(viewport_w) / viewport_h;

    // % of the width and height of the render and resolve buffer we're using. squared this yields our actual memory efficiency at least for this one target.
    render_buffer_usage = actual_supersampling / render_target_metas[q.render_target].max_supersampling_factor;
    resolve_buffer_usage = actual_supersampling / max_supersampling_factor;

    glViewport(0, 0, viewport_w, viewport_h);

    log("frame")->info("rendering stencil");


    if (debug_wireframe_hidden_area) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    distorted.render_stencil();
    if (debug_wireframe_hidden_area) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    log("frame")->info("submitting uniforms");

    submit_uniforms();

    log("frame")->info("rendering skybox");


    glBindVertexArray(dummy_vao);
    if (skybox_visible) sky.render();

    log("frame")->info("rendering controllers");

    controllers.render(controller_mask);

    log("frame")->info("post processing");


    tonemap();


    log("frame")->info("presentation");

    present();

    log("frame")->info("switching to desktop");


    // switch to the 0 frame buffer for drawing to the window
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_MULTISAMPLE);

    // check if window is minimized here.
    auto flags = SDL_GetWindowFlags(window.sdl_window);
    if (flags & SDL_WINDOW_MINIMIZED) continue; // drop gui on the floor?

    log("frame")->info("screen is visible, rendering");


    // ImGui::Image(ImTextureID(sky.testmap), ImVec2(128, 128));

    {
      int w, h;
      SDL_GetWindowSize(window.sdl_window, &w, &h);

      if (debug_wireframe_distortion) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

      // lets find an aspect ratio preserving viewport
      switch (desktop_view) {
        case 0: break;
        case 1: {
          auto dim = fit_viewport(aspect_ratio * 2, w, h);
          glViewport(dim.x, dim.y, dim.w, dim.h);
          distorted.render(resolve_texture,3);
          break;
        }
        case 2: 
        case 3: {
          auto dim = fit_viewport(aspect_ratio, w, h);
          glViewport(dim.x - (desktop_view - 2) * dim.w, dim.y, dim.w * 2, dim.h);
          glScissor(dim.x, dim.y, dim.w, dim.h);
          glEnable(GL_SCISSOR_TEST);
          distorted.render(resolve_texture,desktop_view - 1);
          glDisable(GL_SCISSOR_TEST);
          break;
        }
        case 4: {
          auto dim = fit_viewport(aspect_ratio * 2, w, h);
          glBlitNamedFramebuffer(
            resolve_fbo, 0,
            0, 0,
            viewport_w, viewport_h,
            dim.x, dim.y,
            dim.x + dim.w/2, dim.y + dim.h,
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR
          );
          glBlitNamedFramebuffer(
            resolve_view_fbo, 0,
            0, 0,
            viewport_w, viewport_h,
            dim.x + dim.w/2, dim.y,
            dim.x + dim.w, dim.y + dim.h,
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR
          );

          break;
        }
        case 5: {
          auto dim = fit_viewport(aspect_ratio, w, h);
          glBlitNamedFramebuffer(
            resolve_fbo, 0,
            0 , 0,
            viewport_w, viewport_h,
            dim.x, dim.y,
            dim.x + dim.w, dim.y + dim.h,
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR
          );
          break;
        }

        case 6: {
          auto dim = fit_viewport(aspect_ratio, w, h);
          auto display_shift = (desktop_view - 5) * viewport_w;
          glBlitNamedFramebuffer(
            resolve_fbo, 0, 
            0, 0, 
            viewport_w, viewport_h,
            dim.x, dim.y, 
            dim.x + dim.w, dim.y + dim.h, 
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR
          );
          break;
        }
      }

      if (debug_wireframe_distortion) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    log("frame")->info("showing imgui");

    if (show_gui()) return;

    log("frame")->info("swapping window");

    window.swap();
  }
}

bool app::show_gui(bool * open) {

  if (gui::BeginMainMenuBar()) {
    if (gui::BeginMenu("File")) {
      if (gui::MenuItem("New")) {}
      gui::Separator();
      if (gui::MenuItem("Quit")) return true;
      gui::EndMenu();
    }
    if (gui::BeginMenu("Edit")) {
      if (gui::MenuItem(ICON_MD_UNDO " Undo", "^Z")) {}
      if (gui::MenuItem(ICON_MD_REDO " Redo", "^Y", false, false)) {}  // Disabled item
      gui::Separator();
      if (gui::MenuItem("Cut", "^X")) {}
      if (gui::MenuItem("Copy", "^C")) {}
      if (gui::MenuItem("Paste", "^V")) {}
      gui::EndMenu();
    }
    if (gui::BeginMenu("Scene")) {
      if (gui::MenuItem("Skybox Enabled", nullptr, &skybox_visible)) {}
      gui::EndMenu();

    }
    if (gui::BeginMenu("View")) {
      static const char * view_name[] = {\
        "None",
        "Both Eyes Distorted",
        "Left Eye Distorted",
        "Right Eye Distorted",
        "Both Eyes Raw",
        "Left Eye Raw",
        "Right Eye Raw"
      };
      for (int i = 0; i < countof(view_name); ++i)
        if (gui::MenuItem(view_name[i], nullptr, desktop_view == i))
          desktop_view = i;              
      gui::EndMenu();
    }
    if (gui::BeginMenu("Window")) {      
      if (gui::MenuItem("Settings", nullptr, &show_settings_window)) {}
      if (gui::MenuItem("Timing", nullptr, &show_timing_window)) {}
      if (gui::MenuItem("Demo", nullptr, &show_demo_window)) {}
      gui::EndMenu();
    }
    gui::EndMainMenuBar();
  }

  if (show_settings_window) {
    gui::Begin("Settings", &show_settings_window);
    
    gui::SliderInt("desktop view", &desktop_view, 0, 6);
    gui::SliderInt("min quality", &minimum_quality_level, 0, quality_level_count - 1);
    gui::SliderInt("quality", &quality_level, 0, quality_level_count - 1);
    gui::SliderInt("max quality", &maximum_quality_level, 0, quality_level_count - 1);
    gui::SliderFloat("super-sampling", &desired_supersampling, 0.3, ::max_supersampling_factor);
    gui::Checkbox("force interleaved reprojection", &force_interleaved_reprojection);
    gui::Checkbox("wireframe hidden area", &debug_wireframe_hidden_area);  ImGui::SameLine();
    gui::Checkbox("wireframe distortion", &debug_wireframe_distortion);
    bool tonemap = enable_tonemap; gui::Checkbox("tonemap", &tonemap); enable_tonemap = tonemap;  ImGui::SameLine();
    bool seascape = enable_seascape; gui::Checkbox("seascape", &seascape); enable_seascape = seascape;
    gui::End();
  }


  if (gui::Button(suspended_rendering ? "Resume VR" : "Suspend VR")) {
    suspended_rendering = !suspended_rendering;
    vr::VRCompositor()->SuspendRendering(suspended_rendering);
  }

  if (show_demo_window)
    gui::ShowTestWindow(&show_demo_window);

  gui::Render();
  return false;
}

int SDL_main(int argc, char ** argv) {
  auto test = sampled_spectrum::from_rgb(vec3(1, 0.5, 0.5));
  spdlog::set_pattern("%a %b %m %Y %H:%M:%S.%e - %n %l: %v"); // [thread %t]"); // close enough to the native notifications from openvr that the debug log is readable.
#ifdef _WIN32
  SetProcessDPIAware(); // if we don't call this, then SDL2 will lie and always tell us that DPI = 96
#endif

  log("app")->info("process id: {}", GetCurrentProcessId());

  path exe = executable_path();
  log("app")->info("path: {}", exe);

  path asset_dir = path(exe.parent_path().parent_path().parent_path()).append("assets");
  log("app")->info("assets: {}", asset_dir);

  _wchdir(asset_dir.native().c_str());

  cds_main_thread_attachment<> main_thread; // Allow use of concurrent data structures in the main threads


  app main;
  main.run();

  spdlog::details::registry::instance().apply_all([](shared_ptr<logger> logger) { logger->flush(); }); // make sure the logs are flushed before shutting down
  spdlog::details::registry::instance().drop_all(); // allow any dangling logs with no references to more gracefully shutdown
  return 0;
}