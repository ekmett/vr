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
#include "quality.h"

using namespace framework;
using namespace filesystem;

// used reversed [1..0] floating point z rather than the classic [-1..1] mapping
#define USE_REVERSED_Z

#ifdef USE_REVERSED_Z
static float reverseZ_contents[16] = {
  1.f, 0.f, 0.f, 0.f,
  0.f, 1.f, 0.f, 0.f,
  0.f, 0.f, -1.f, 1.f,
  0.f, 0.f, 1.f, 0.f
};

static mat4 reverseZ = make_mat4<float>(reverseZ_contents);
#endif

struct viewport_dim {
  GLint x, y, w, h;
};

viewport_dim fit_viewport(float aspectRatio, int w, int h, bool bottom_justified = true, bool left_justified = false) {
  GLint x = GLint(h * aspectRatio);
  if (x <= w) {
    return viewport_dim{ left_justified ? 0 : (w - x) / 2, 0, x, h };
  } else {
    GLint y = GLint(w / aspectRatio);
    return viewport_dim{ 0, bottom_justified ? 0 : (h - y) / 2, w, y };
  }
}

struct app : app_uniforms {
  app();
  ~app();

  void run();
  void submit_uniforms() {
    global_time = SDL_GetTicks() / 1000.0f;
    glNamedBufferSubData(ubo, 0, sizeof(app_uniforms), static_cast<app_uniforms*>(this));
  }
 
  sdl::window window; // must come before anything that needs opengl support in this object, implicitly supplies gl context
  openvr::system vr;  // must come before anything that uses openvr in this object, implicitly supplies vr::VRSystem(), etc.
  openal::system al;  // must come before anything that uses sound

  quality quality;    // requires vr
  gl::compiler compiler;
  GLuint ubo;
  GLuint dummy_vao;
  framework::sky sky;
  distortion distorted;

  bool debug_wireframe_hidden_area = false;
  bool debug_wireframe_distortion = false;
  bool show_settings_window = true;
  bool show_demo_window = false;
  bool suspended_rendering = false;
  
  gui::system gui;
  controllers controllers;
  std::mt19937 rng; // for the main thread  
  int desktop_view = 1;
  bool skybox_visible = true;
  int controller_mask = 0;
  
private: 
  // void initialize_framebuffers();
  void get_poses();
  void update_controller_assignment();
  bool show_gui(bool * open = nullptr);
  void desktop_display();
};


app::app()
  : window("proc", { 4, 5, gl::profile::core }, true, 50, 50, 1280, 1024)
  , vr()
  , compiler(path("shaders"))
  , gui(window)
  , distorted()
  , sky(vec3(0.0, 0.1, 0.8), 2.0_degrees, vec3(0.2, 0.2, 0.4), 6.f, *this)
  //, sky(vec3(0.252, 0.955, -.155), 2.0_degrees, vec3(0.25, 0.25, 0.25), 2.f, *this)
  , quality(3)
  {

  nearClip = 0.1f;
  farClip = 10000.f;
  
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

  SDL_DisplayMode wdm;
  SDL_GetWindowDisplayMode(window.sdl_window, &wdm);
  log("app")->info("desktop window refresh rate: {}hz", wdm.refresh_rate); // TODO: compute frame skipping for gui and desktop display off this relative to the vr rate.

  SDL_StartTextInput();
  submit_uniforms(); // pre-load some data
}

app::~app() {
  SDL_StopTextInput();
  glDeleteVertexArrays(1, &dummy_vao);
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

void app::get_poses() {
  vr::TrackedDevicePose_t physical_pose[vr::k_unMaxTrackedDeviceCount]; // current poses
  vr::TrackedDevicePose_t predicted_pose[vr::k_unMaxTrackedDeviceCount]; // poses 2 frames out

  vr::VRCompositor()->WaitGetPoses(physical_pose, vr::k_unMaxTrackedDeviceCount, predicted_pose, vr::k_unMaxTrackedDeviceCount);

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

    gui.new_frame();

    // gather uniforms
    get_poses();
    quality.new_frame(vr, &render_buffer_usage, &resolve_buffer_usage);

    submit_uniforms();

    distorted.render_stencil(debug_wireframe_hidden_area);

    if (skybox_visible) {
      glBindVertexArray(dummy_vao);
      sky.render();
    }

    controllers.render(controller_mask);

    quality.present();
    desktop_display();
  }
}

void app::desktop_display() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_MULTISAMPLE);
  auto flags = SDL_GetWindowFlags(window.sdl_window);

  if (flags & SDL_WINDOW_MINIMIZED) {
    glFlush();
    return; // drop gui on the floor?
  }

  // clear the display window  
  glClearColor(0.18f, 0.18f, 0.18f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  int w, h;
  SDL_GetWindowSize(window.sdl_window, &w, &h);

  if (debug_wireframe_distortion) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  // lets find an aspect ratio preserving viewport
  switch (desktop_view) {
    case 0: break;
    case 1: {
      auto dim = fit_viewport(quality.aspect_ratio * 2, w, h);
      glViewport(dim.x, dim.y, dim.w, dim.h);
      distorted.render(quality.resolve_texture, 3);
      break;
    }
    case 2:
    case 3: {
      auto dim = fit_viewport(quality.aspect_ratio, w, h);
      glViewport(dim.x - (desktop_view - 2) * dim.w, dim.y, dim.w * 2, dim.h);
      glScissor(dim.x, dim.y, dim.w, dim.h);
      glEnable(GL_SCISSOR_TEST);
      distorted.render(quality.resolve_texture, desktop_view - 1);
      glDisable(GL_SCISSOR_TEST);
      break;
    }
    case 4: {
      auto dim = fit_viewport(quality.aspect_ratio * 2, w, h);
      glBlitNamedFramebuffer(
        quality.resolve_fbo, 0,
        0, 0,
        quality.viewport_w, quality.viewport_h,
        dim.x, dim.y,
        dim.x + dim.w / 2, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      glBlitNamedFramebuffer(
        quality.resolve_view_fbo, 0,
        0, 0,
        quality.viewport_w, quality.viewport_h,
        dim.x + dim.w / 2, dim.y,
        dim.x + dim.w, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );

      break;
    }
    case 5: {
      auto dim = fit_viewport(quality.aspect_ratio, w, h);
      glBlitNamedFramebuffer(
        quality.resolve_fbo, 0,
        0, 0,
        quality.viewport_w, quality.viewport_h,
        dim.x, dim.y,
        dim.x + dim.w, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      break;
    }

    case 6: {
      auto dim = fit_viewport(quality.aspect_ratio, w, h);
      auto display_shift = (desktop_view - 5) * quality.viewport_w;
      glBlitNamedFramebuffer(
        quality.resolve_fbo, 0,
        0, 0,
        quality.viewport_w, quality.viewport_h,
        dim.x, dim.y,
        dim.x + dim.w, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      break;
    }
  }

  if (debug_wireframe_distortion) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  if (show_gui()) return;

  glFlush();
  //static int frame = 0;
  //if (frame++ % 3 == 0) 
  window.swap();
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
      if (gui::MenuItem("Quality", nullptr, &quality.show_quality_window)) {}
      if (gui::MenuItem("Timing", nullptr, &quality.show_timing_window)) {}
      if (gui::MenuItem("Demo", nullptr, &show_demo_window)) {}
      gui::EndMenu();
    }
    gui::EndMainMenuBar();
  }

  if (show_settings_window) {
    gui::Begin("Settings", &show_settings_window);    
    gui::SliderInt("desktop view", &desktop_view, 0, 6);
    gui::Checkbox("wireframe hidden area", &debug_wireframe_hidden_area);  ImGui::SameLine();
    gui::Checkbox("wireframe distortion", &debug_wireframe_distortion);
    bool tonemap = enable_tonemap; gui::Checkbox("tonemap", &tonemap); enable_tonemap = tonemap;  ImGui::SameLine();
    bool seascape = enable_seascape; gui::Checkbox("seascape", &seascape); enable_seascape = seascape;
    gui::End();
  }

  if (gui::Button(quality.suspended_rendering ? "Resume VR" : "Suspend VR")) {
    quality.suspended_rendering = !quality.suspended_rendering;
    vr::VRCompositor()->SuspendRendering(quality.suspended_rendering);
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

  log("app")->info("pid: {}", GetCurrentProcessId());

  path exe = executable_path();
  path asset_dir = path(exe.parent_path().parent_path().parent_path()).append("assets");
  _wchdir(asset_dir.native().c_str());

  cds_main_thread_attachment<> main_thread; // Allow use of concurrent data structures in the main threads

  app main;
  main.run();

  spdlog::details::registry::instance().apply_all([](shared_ptr<logger> logger) { logger->flush(); }); // make sure the logs are flushed before shutting down
  spdlog::details::registry::instance().drop_all(); // allow any dangling logs with no references to more gracefully shutdown
  return 0;
}