#include "stdafx.h"
#include <random>
#include <functional>
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

// valve-style adaptive rendering quality

static const struct render_target_meta {
  int msaa_level;
  float max_supersampling_factor;
} render_target_metas[] {
  { 4, 1.1 },
  { 8, 1.4 }
};

static const int render_target_count = countof(render_target_metas);
static const float max_supersampling_factor = 1.4; 

static const struct quality_level {
  int render_target;
  float resolution_scale;
} quality_levels[] = {
  { 0, 0.4 },
  { 0, 0.5 },
  { 0, 0.65 }, // 0, valve's -4 
  { 0, 0.73 }, // 1, valve -3
  { 0, 0.81 }, // 2, valve -2
  { 0, 0.9 },  // 3, valve -1
  { 0, 1.0 },
  { 0, 1.1 },  // largest 4x supersampling factor = 1.1
  { 1, 1.0 },
  { 1, 1.1 },
  { 1, 1.2 },
  { 1, 1.3 },
  { 1, 1.4 }   // largest 8x supersampling factor = 1.4
};

static const int quality_level_count = countof(quality_levels);

struct viewport_dim {
  GLint x, y, w, h;
};

// used reversed [1..0] floating point z rather than the classic [-1..1] mapping
#define USE_REVERSED_Z

struct app : app_uniforms {
  app();
  ~app();

  void run();
  bool show_gui(bool * open = nullptr);
  void get_poses();
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
  bool show_debug_window = true;
  bool show_demo_window = false;

  gui::system gui;
  //overlay dashboard;
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

    //uint32_t w, h;
  union {
    GLuint fbo[render_target_count + 1];
    struct {
      GLuint render_fbo[render_target_count], resolve_fbo;
    };
  };
  GLuint depth_target[render_target_count];  // renderbuffer for render framebuffer
  union {
    GLuint texture[render_target_count + 1];
    struct {
      GLuint render_texture[render_target_count], resolve_texture;
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
  , sky(vec3(0.0, 0.1, 0.8), 2.0_degrees, vec3(0.2, 0.2, 0.4), 6.f, *this)
  , debug_wireframe_hidden_area(false)
  {

  nearClip = 0.1f;
  farClip = 10000.f;
  quality_level = 6;

  glCreateVertexArrays(1, &dummy_vao);
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

  // set up all the rendering targets  
  vr.handle->GetRecommendedRenderTargetSize(&recommended_w, &recommended_h);
  glCreateFramebuffers(countof(fbo), fbo);
  glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 2, render_texture);
  glCreateRenderbuffers(countof(depth_target), depth_target);
  for (int i = 0;i < render_target_count; ++i) {
    auto meta = render_target_metas[i];
    gl::label(GL_FRAMEBUFFER, render_fbo[i], "render fbo {}", i);
    gl::label(GL_RENDERBUFFER, depth_target[i], "render depth/stencil {}", i);
    gl::label(GL_TEXTURE, render_texture[i], "render texture {}", i);

    glNamedRenderbufferStorageMultisample(
      depth_target[i],
      meta.msaa_level,
      GL_DEPTH32F_STENCIL8,
      recommended_w * 2 * meta.max_supersampling_factor,
      recommended_h * meta.max_supersampling_factor
    );

    glNamedFramebufferRenderbuffer(render_fbo[i], GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_target[i]);

    glTextureImage2DMultisampleNV(
      render_texture[i],
      GL_TEXTURE_2D_MULTISAMPLE,
      meta.msaa_level,
      GL_RGBA16F,
      recommended_w * 2 * meta.max_supersampling_factor,
      recommended_h * meta.max_supersampling_factor,
      true
    );

    glTextureParameteri(render_texture[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(render_texture[i], GL_TEXTURE_MAX_LEVEL, 0);
    glNamedFramebufferTexture(render_fbo[i], GL_COLOR_ATTACHMENT0, render_texture[i], 0);
  }
 
  gl::label(GL_FRAMEBUFFER, resolve_fbo, "resolve fbo");
  glCreateTextures(GL_TEXTURE_2D, 1, &resolve_texture);

  gl::label(GL_TEXTURE, resolve_texture, "resolve texture");
  glTextureParameteri(resolve_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(resolve_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(resolve_texture, GL_TEXTURE_MAX_LEVEL, 0);
  glTextureParameteri(resolve_texture, GL_TEXTURE_WRAP_S, GL_REPEAT); // CLAMP_TO_EDGE);
  glTextureParameteri(resolve_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // todo: grab a handle

  resolve_buffer_w = recommended_w * 2 * max_supersampling_factor;
  resolve_buffer_h = recommended_h * max_supersampling_factor;
  
  glTextureImage2DEXT(resolve_texture, GL_TEXTURE_2D, 0, GL_RGBA8, resolve_buffer_w, resolve_buffer_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glNamedFramebufferTexture(resolve_fbo, GL_COLOR_ATTACHMENT0, resolve_texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    die("Unable to allocate frame buffer");

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
  glDeleteRenderbuffers(countof(depth_target), depth_target);
  glDeleteTextures(countof(texture), texture);
}


viewport_dim fit_viewport(float aspectRatio, int w, int h, bool bottom_justified = true, bool left_justified = false) {
  GLint x = GLint(h * aspectRatio);
  if (x <= w) {
    return viewport_dim { left_justified ? 0 : (w-x)/2, 0, x, h };
  } else {
    GLint y = GLint(w / aspectRatio);
    return viewport_dim { 0, bottom_justified ? 0 : (h-y)/2, w, y };
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
  for (int i = 0;i < 2;++i) {
    auto v = viewports[i];
    glBlitNamedFramebuffer(
      render_fbo[q.render_target],
      resolve_fbo,
      v.x, v.y, v.x + v.w, v.y + v.h,
      v.x, v.y, v.x + v.w, v.y + v.h,
      GL_COLOR_BUFFER_BIT,
      GL_LINEAR
    );
    log("debug")->info("tonemap {}: {},{},{},{} render buffer usage: {}, resolve: {}", i, v.x, v.y, v.x + v.w, v.y + v.h, render_buffer_usage, resolve_buffer_usage );

  }
}
void app::present() {
  vr::Texture_t eyeTexture{ (void*)(intptr_t)(resolve_texture), vr::API_OpenGL, vr::ColorSpace_Gamma };
  for (int i = 0;i < 2;++i) {
    auto v = viewports[i];    
    vr::VRTextureBounds_t eyeBounds = { 
       v.x / float(resolve_buffer_w), 
       1 - (v.y + v.h) / float(resolve_buffer_h),
       (v.x + v.w) / float(resolve_buffer_w), 
       1 - v.y / float(resolve_buffer_h)
    };
    vr::VRCompositor()->Submit(vr::EVREye(i), &eyeTexture, &eyeBounds); // NB: if we do the distortion ourselves we could do it in hdr
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

void app::run() {
  while (!vr.poll() && !window.poll()) {
    // clear the display window  
    glClearColor(0.18f, 0.18f, 0.18f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // start a new imgui frame
    gui.new_frame();

    // hmd
    glEnable(GL_MULTISAMPLE);
    auto q = quality_levels[quality_level];
    glBindFramebuffer(GL_FRAMEBUFFER, render_fbo[q.render_target]);
    glStencilMask(1);
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    GLint viewport_w = recommended_w * q.resolution_scale,
          viewport_h = recommended_h * q.resolution_scale;
    float aspect_ratio = float(viewport_w) / viewport_h;

    // % of the width and height of the render and resolve buffer we're using. squared this yields our actual memory efficiency
    render_buffer_usage = q.resolution_scale / render_target_metas[q.render_target].max_supersampling_factor;
    resolve_buffer_usage = q.resolution_scale / max_supersampling_factor ;

    glViewport(0, 0, viewport_w * 2, viewport_h);
    if (debug_wireframe_hidden_area) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    distorted.render_stencil();
    if (debug_wireframe_hidden_area) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glViewportIndexedf(0, 0, 0, (float)viewport_w, (float)viewport_h);                // viewport 0 left eye
    viewports[0] = { 0, 0, viewport_w, viewport_h };

    glViewportIndexedf(1, (float)viewport_w, 0, (float)viewport_w, (float)viewport_h); // viewport 1 right eye
    viewports[1] = { viewport_w, 0, viewport_w, viewport_h };
    
    get_poses();
    submit_uniforms();

    glBindVertexArray(dummy_vao);
    if (skybox_visible) sky.render();

    controllers.render(controller_mask);
    tonemap();
    present();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_MULTISAMPLE);

    // check if window is minimized here.
    auto flags = SDL_GetWindowFlags(window.sdl_window);
    if (flags & SDL_WINDOW_MINIMIZED) continue; // drop gui on the floor?

    //ImGui::Text("HMD Position: %3.2f %3.2f %3.2f", hmdToWorld[3][0], hmdToWorld[3][1], hmdToWorld[3][2]);

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
          distorted.render(resolve_texture);
          break;
        }
        case 2: 
        case 3: {
          auto dim = fit_viewport(aspect_ratio, w, h);
          glViewport(dim.x - (desktop_view - 2) * dim.w, dim.y, dim.w * 2, dim.h);
          glScissor(dim.x, dim.y, dim.w, dim.h);
          glEnable(GL_SCISSOR_TEST);
          distorted.render(resolve_texture);
          glDisable(GL_SCISSOR_TEST);
          break;
        }
        case 4: {
          auto dim = fit_viewport(aspect_ratio * 2, w, h);
          glBlitNamedFramebuffer(
            resolve_fbo, 0,
            0, 0,
            viewport_w*2, viewport_h,
            dim.x, dim.y,
            dim.x + dim.w, dim.y + dim.h,
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR
          );
          break;
        }
        case 5: 
        case 6: {
          auto dim = fit_viewport(aspect_ratio, w, h);
          auto display_shift = (desktop_view - 5) * viewport_w;
          glBlitNamedFramebuffer(
            resolve_fbo, 0, 
            display_shift, 0, 
            display_shift + viewport_w, viewport_h,
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

    if (show_gui()) return;

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
      if (gui::MenuItem("Debug", nullptr, &show_debug_window)) {}
      if (gui::MenuItem("Demo", nullptr, &show_demo_window)) {}
      gui::EndMenu();
    }
    gui::EndMainMenuBar();
  }

  if (show_debug_window) {
    gui::Begin("Debug", &show_debug_window);
    gui::SliderInt("quality", &quality_level, 0, quality_level_count - 1); 
    gui::SliderInt("desktop view", &desktop_view, 0, 6);
    gui::Checkbox("wireframe hidden area", &debug_wireframe_hidden_area);
    gui::Checkbox("wireframe distortion", &debug_wireframe_distortion);
    bool tonemap = enable_tonemap; gui::Checkbox("tonemap", &tonemap); enable_tonemap = tonemap;
    bool seascape = enable_seascape; gui::Checkbox("seascape", &seascape); enable_seascape = seascape;   
    gui::End();
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