#include "stdafx.h"
#include <random>
#include <functional>
#include <algorithm>
#include "framework/worker.h"
#include "framework/gl.h"
#include "framework/signal.h"
#include "framework/filesystem.h"
#include "framework/gui.h"
#include <glm/gtc/matrix_transform.hpp>
#include "framework/openal.h"
#include "framework/distortion.h"
//#include "overlay.h"
#include "framework/skybox.h"
#include "framework/spectrum.h"
#include "framework/quality.h"
#include "framework/post.h"
#include "framework/rendermodel.h"
#include "uniforms.h"
#include "controllers.h"

using namespace framework;
using namespace filesystem;

// used reversed [1..0] floating point z rather than the classic [-1..1] mapping
//#define USE_REVERSED_Z

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

static const char * desktop_views[] = { \
"None",
"Both Eyes Distorted",
"Left Eye Distorted",
"Right Eye Distorted",
"Both Eyes Raw",
"Left Eye Raw",
"Right Eye Raw",
"Presolve Buffer",
"Post 0",
"Post 1"
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
  app(path assets);
  ~app();

  void run();
  void submit_uniforms() {
    global_time = SDL_GetTicks() / 1000.0f;
    glNamedBufferSubData(ubo, 0, sizeof(app_uniforms), static_cast<app_uniforms*>(this));
  }

  void calculate_composite_frustum();
 
  sdl::window window; // must come before anything that needs opengl support in this object, implicitly supplies gl context
  gl::compiler compiler; // must come before anything that uses includes in opengl
  openvr::system vr;  // must come before anything that uses openvr in this object, implicitly supplies vr::VRSystem(), etc.
  openal::system al;  // must come before anything that uses sound
  rendermodel_manager rendermodels;

  quality quality;    // requires vr
  post post;          // post processor, requires quality
  GLuint ubo;
  GLuint dummy_vao;
  framework::sky sky;
  distortion distorted;

  bool show_settings_window = true;
  bool show_demo_window = false;
  bool show_rendermodel_window = true;
  
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
  bool show_gui(bool * open = nullptr); // returns if we should shut down
  bool desktop_display();
};


app::app(path assets)
  : window("proc", { 4, 5, gl::profile::core }, true, 50, 50, 1280, 1024)
  , vr()
  , compiler(path(assets).append("shaders"))
  , gui(window)
  // , sky(vec3(0.252, 0.955, -.155), 2.0_degrees, vec3(0.25, 0.25, 0.25), 2.f, *this)
  , quality(3)
  , post(quality)
  , rendermodels(vr)
  {
  nearClip = 0.1f;
  farClip = 10000.f;
  bloom_exposure = -8.125;
  exposure = -14;
  blur_sigma = 2.5;
  bloom_magnitude = 1.000;

  sun_dir = vec3(0.228, 0.342, 0.912);
  sun_angular_radius = 2 * physical_sun_angular_radius;
  ground_albedo = vec3(0.25, 0.25, 0.25); 
  turbidity = 3.f;
  use_sun_area_light_approximation = true;

  distorted.set_resolve_handle(quality.resolve_target.texture_handle);
  
  enable_seascape = true;
  use_sun_area_light_approximation = true;

  rendermodel_metallic = 0.1;
  rendermodel_roughness = 0.05;
  rendermodel_ambient = 0.8; // bug open scene
  rendermodel_albedo = 1;

  glCreateVertexArrays(1, &dummy_vao); // we'll load this as needed
  gl::label(GL_VERTEX_ARRAY, dummy_vao, "dummy vao");

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

  calculate_composite_frustum();
  submit_uniforms(); // pre-load some data
  SDL_StartTextInput();
}

app::~app() {
  SDL_StopTextInput();
  glDeleteVertexArrays(1, &dummy_vao);
}


void app::calculate_composite_frustum() {
  struct frustum { 
    float l, r, t, b; 
  } e[2];
  for (int i = 0;i < 2;++i) {
    vr::VRSystem()->GetProjectionRaw(vr::EVREye(i), &e[i].l, &e[i].r, &e[i].t, &e[i].b);
    log("app")->info("raw frustum {}: {}, {}, {}, {}", i, e[i].l, e[i].r, e[i].t, e[i].b);
  }
  vec2 tan_half(
    max({ -e[0].l, e[0].r,-e[1].l,e[1].r }),
    max({ -e[0].t, e[0].b,-e[1].t,e[1].b })
  );

  float aspect_ratio = tan_half.x / tan_half.y;
  float fov = 2 * atan(tan_half.y) * 180.0f / float(M_PI);

  log("app")->info("composite frustum has an {} degree fov, aspect ratio: {}", fov, aspect_ratio);

  struct bounds {
    float ul, uh, vl, vh;
  } t[2];

  for (int i = 0;i < 2;++i) {
    t[i] = {
      0.5f + 0.5f * e[i].l / tan_half.x,
      0.5f + 0.5f * e[i].r / tan_half.x,
      0.5f - 0.5f * e[i].t / tan_half.y,
      0.5f - 0.5f * e[i].b / tan_half.y
    };
    log("app")->info("viewport {} ({},{}) - ({},{})", i, t[i].ul, t[i].vh, t[i].uh, t[i].vl);
  }

  uint32_t w, h;
  vr::VRSystem()->GetRecommendedRenderTargetSize(&w, &h);

  log("app")->info("individual recommended window size: {} x {}", w, h);
  w = uint32_t(w / std::max(t[0].uh - t[0].ul, t[1].uh - t[1].ul));
  h = uint32_t(h / std::max(t[0].uh - t[0].ul, t[1].uh - t[1].ul));
  log("app")->info("composite recommended window size: {} x {}", w, h);
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

  device_mask = 0;
  for (int i = 0;i < vr::k_unMaxTrackedDeviceCount; ++i) {
    current_device_to_world[i] = openvr::hmd_mat3x4(physical_pose[i].mDeviceToAbsoluteTracking);
    predicted_device_to_world[i] = openvr::hmd_mat3x4(predicted_pose[i].mDeviceToAbsoluteTracking);
    device_mask |= physical_pose[i].bPoseIsValid ? (1 << i) : 0;

    auto vector4 = [](float v[3]) -> vec4 { return vec4(v[0], v[1], v[2], 0); };

    current_device_angular_velocity[i] = vector4(physical_pose[i].vAngularVelocity.v);
    predicted_device_angular_velocity[i] = vector4(physical_pose[i].vAngularVelocity.v);
    current_device_velocity[i] = vector4(physical_pose[i].vVelocity.v);
    predicted_device_velocity[i] = vector4(physical_pose[i].vVelocity.v);
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
    auto l = log("app");

    l->info("gui frame");
    gui.new_frame();

    l->info("polling rendermodels");
    rendermodels.poll();

    l->info("get_poses");
    get_poses();

    l->info("quality.new_frame");
    quality.new_frame(vr, &render_buffer_usage, &resolve_buffer_usage);

    sky.update(*this);

    l->info("submit_uniforms");
    submit_uniforms();

    l->info("render_stencil");

    distorted.render_stencil();

    l->info("skybox");
    glBindVertexArray(dummy_vao);
    sky.render();

    l->info("drawing rendermodels");
    rendermodels.draw(device_mask);

    //l->info("drawing control vectors");
    //controllers.render(controller_mask);

    l->info("post");
    quality.resolve(post.presolve);
    post.process();

    l->info("present");
    quality.present();

    l->info("desktop");
    
    if (desktop_display()) return;
  }
}

bool app::desktop_display() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_MULTISAMPLE);
  auto flags = SDL_GetWindowFlags(window.sdl_window);

  if (flags & SDL_WINDOW_MINIMIZED) {
    glFlush();
    return false; // drop gui on the floor?
  }

  int w, h;
  SDL_GetWindowSize(window.sdl_window, &w, &h);

  glDisable(GL_STENCIL_TEST);
  glStencilMask(1);
  glViewport(0, 0, w, h);
  glScissor(0, 0, w, h);
  glClearColor(0.18f, 0.18f, 0.18f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // lets find an aspect ratio preserving viewport
  switch (desktop_view) {
    case 0: break;
    case 1: {
      auto dim = fit_viewport(quality.aspect_ratio * 2, w, h);
      glViewport(dim.x, dim.y, dim.w, dim.h);
      distorted.render(3);
      break;
    }
    case 2:
    case 3: {
      auto dim = fit_viewport(quality.aspect_ratio, w, h);
      glViewport(dim.x - (desktop_view - 2) * dim.w, dim.y, dim.w * 2, dim.h);
      glScissor(dim.x, dim.y, dim.w, dim.h);
      glEnable(GL_SCISSOR_TEST);
      distorted.render(desktop_view - 1);
      glDisable(GL_SCISSOR_TEST);
      break;
    }
    case 4: {
      auto dim = fit_viewport(quality.aspect_ratio * 2, w, h);
      glBlitNamedFramebuffer(
        quality.resolve_target.fbo_view[0], 0,
        0, 0,
        quality.viewport_w, quality.viewport_h,
        dim.x, dim.y,
        dim.x + dim.w / 2, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      glBlitNamedFramebuffer(
        quality.resolve_target.fbo_view[1], 0,
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
        quality.resolve_target.fbo_view[0], 0,
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
      glBlitNamedFramebuffer(
        quality.resolve_target.fbo_view[1], 0,
        0, 0,
        quality.viewport_w, quality.viewport_h,
        dim.x, dim.y,
        dim.x + dim.w, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      break;
    }
    case 7: {
      auto dim = fit_viewport(quality.aspect_ratio, w, h);
      glBlitNamedFramebuffer(
        post.presolve.fbo, 0,
        0, 0,
        quality.viewport_w, quality.viewport_h,
        dim.x, dim.y,
        dim.x + dim.w, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      break;
    }
    case 8: {
      auto dim = fit_viewport(quality.aspect_ratio, w, h);
      glBlitNamedFramebuffer(
        post.fbo[0].fbo, 0,
        0, 0,
        post.pw, post.ph,
        dim.x, dim.y,
        dim.x + dim.w, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      break;
    }
    case 9: {
      auto dim = fit_viewport(quality.aspect_ratio, w, h);
      glBlitNamedFramebuffer(
        post.fbo[1].fbo, 0,
        0, 0,
        post.pw, post.ph,
        dim.x, dim.y,
        dim.x + dim.w, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      break;
    }
  }

  glViewport(0, 0, w, h);

  if (show_gui()) return true;

  glFlush();
  window.swap();

  return false;
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
    if (gui::BeginMenu("View")) {
      for (int i = 0; i < countof(desktop_views); ++i)
        if (gui::MenuItem(desktop_views[i], nullptr, desktop_view == i))
          desktop_view = i;
      gui::EndMenu();
    }
    if (gui::BeginMenu("Debug")) {
      if (gui::BeginMenu("Wireframe")) {
        if (gui::BeginMenu("Distortion")) {
          gui::MenuItem("Render", nullptr, &distorted.debug_wireframe_render);
          gui::MenuItem("Render Stencil", nullptr, &distorted.debug_wireframe_render_stencil);
          gui::EndMenu();
        }
        gui::EndMenu();
      }
      gui::Separator();        
      gui::MenuItem("Skybox Enabled", nullptr, &skybox_visible);
      gui::Separator();
      gui::MenuItem("Settings", nullptr, &show_settings_window);
      gui::MenuItem("Quality", nullptr, &quality.show_quality_window);
      gui::MenuItem("Timing", nullptr, &quality.show_timing_window);
      gui::MenuItem("Demo", nullptr, &show_demo_window);
      gui::MenuItem("Skybox", nullptr, &sky.show_skybox_window);
      gui::MenuItem("Render Models", nullptr, &show_rendermodel_window);
      gui::EndMenu();
    }

    gui::EndMainMenuBar();
  }

  if (show_settings_window) {
    gui::Begin("Settings", &show_settings_window);    
    gui::SliderInt("desktop view", &desktop_view, 0, countof(desktop_views) - 1);
    gui::Text("%s", desktop_views[desktop_view]);
    bool seascape = enable_seascape; gui::Checkbox("seascape", &seascape); enable_seascape = seascape;
    gui::SliderFloat("exposure", &exposure, -30, 30);
    gui::SliderFloat("bloom exposure", &bloom_exposure, -30, 30);
    gui::SliderFloat("bloom magnitude", &bloom_magnitude, 0, 10);
    gui::SliderFloat("blur sigma", &blur_sigma, 0, 10);
    gui::End();
  }

  if (show_rendermodel_window) {
    gui::Begin("Rendermodels");
    gui::SliderFloat("roughness", &rendermodel_roughness, 0, 1);
    gui::SliderFloat("metallic",  &rendermodel_metallic, 0, 1);
    gui::SliderFloat("ambient",   &rendermodel_ambient, 0, 1);
    gui::SliderFloat("albedo",    &rendermodel_albedo, 0, 1);
    bool sun_area = use_sun_area_light_approximation;
    gui::Checkbox("sun area light", &sun_area);
    use_sun_area_light_approximation = sun_area;
    gui::End();
  }

  if (gui::Button(quality.suspended_rendering ? "Resume VR" : "Suspend VR")) {
    quality.suspended_rendering = !quality.suspended_rendering;
    vr::VRCompositor()->SuspendRendering(quality.suspended_rendering);
  }
  gui::SameLine();

  if (vr::VRCompositor()->IsMirrorWindowVisible()) {
    if (gui::Button("Hide Compositor")) vr::VRCompositor()->HideMirrorWindow();
  } else {
    if (gui::Button("Show Compositor")) vr::VRCompositor()->ShowMirrorWindow();
  }

  if (show_demo_window)
    gui::ShowTestWindow(&show_demo_window);

  gui::Render();
  return false;
}

int SDL_main(int argc, char ** argv) {
   spdlog::set_pattern("%a %b %m %Y %H:%M:%S.%e - %n %l: %v"); // [thread %t]"); // close enough to the native notifications from openvr that the debug log is readable.

  shared_ptr<spdlog::logger> ignore_logs[] {
    spdlog::create<spdlog::sinks::null_sink_mt>("vr"),
  //  spdlog::create<spdlog::sinks::null_sink_mt>("gl"),
    spdlog::create<spdlog::sinks::null_sink_mt>("al"),
    spdlog::create<spdlog::sinks::null_sink_mt>("main"),
   // spdlog::create<spdlog::sinks::null_sink_mt>("app"),
    spdlog::create<spdlog::sinks::null_sink_mt>("post"),
    spdlog::create<spdlog::sinks::null_sink_mt>("quality"),
    spdlog::create<spdlog::sinks::null_sink_mt>("distortion"),
    spdlog::create<spdlog::sinks::null_sink_mt>("rendermodel"),

  };
#ifdef _WIN32
  SetProcessDPIAware(); // if we don't call this, then SDL2 will lie and always tell us that DPI = 96
#endif

  log("main")->info("pid: {}", GetCurrentProcessId());

  path exe = executable_path();
  path asset_dir = path(exe.parent_path().parent_path().parent_path()).append("assets");
  _wchdir(asset_dir.native().c_str());

  cds_main_thread_attachment<> main_thread; // Allow use of concurrent data structures in the main threads

  app main(asset_dir);
  main.run();

  spdlog::details::registry::instance().apply_all([](shared_ptr<logger> logger) { logger->flush(); }); // make sure the logs are flushed before shutting down
  spdlog::details::registry::instance().drop_all(); // allow any dangling logs with no references to more gracefully shutdown
  return 0;
}