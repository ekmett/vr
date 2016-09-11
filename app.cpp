#include "stdafx.h"
#include <algorithm>
#include <functional>
#include <omp.h>
#include <random>
#include "distortion.h"
#include "gl.h"
#include "gui.h"
#include "filesystem.h"
#include "openal.h"
#include "post.h"
#include "quality.h"
#include "rendermodel.h"
#include "signal.h"
#include "skybox.h"
#include "spectrum.h"
#include "timer.h"
#include "worker.h"
#include "cds.h"
#include "shaders/uniforms.h"
#include "controllers.h"
#include "sampling.h"

using namespace framework;
using namespace filesystem;

static const int omp_threads = 6;

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
  app();
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
  bool show_rendermodel_window = false;
  bool show_controllers_window = false;
  bool gl_finish_hack = true;
  bool read_pixel_hack = false;
  
  gui::system gui;
  controllers controllers;
  std::mt19937 rng; // for the main thread  
  int desktop_view = 1;
  bool skybox_visible = true;
  int controller_mask = 0;
  float last_resolve_buffer_usage = 0.f;
  
private: 
  // void initialize_framebuffers();
  void get_poses();
  void update_controller_assignment();
  bool show_gui(bool * open = nullptr); // returns if we should shut down
  void desktop_display();
};


app::app()
  : window("framework", { 4, 5, gl::profile::core }, true, 50, 50, 1280, 1024)
  , vr()
  , compiler(path("shaders"))
  , gui(window)
  , quality(3)
  , post(quality)
  , rendermodels(vr)
  {
  nearClip = 0.1f;
  farClip = 50.f;
  bloom_exposure = -6.f;
  exposure = -14.f;
  blur_sigma = 2.5f;
  bloom_magnitude = 1.000f;
  quality.maximum_quality_level = 4;

  sun_dir = vec3(0.228f, 0.242f, 0.912f);
  sun_angular_radius = physical_sun_angular_radius * 2;
  ground_albedo = vec3(0.25f, 0.4f, 0.4f); 
  turbidity = 5.f;
  use_sun_area_light_approximation = true;
  
  enable_seascape = true;
  use_sun_area_light_approximation = true;

  use_lens_flare = false;
  lens_flare_exposure = -3.5f;
  lens_flare_halo_radius = 0.433f;
  lens_flare_ghosts = 8;
  lens_flare_ghost_dispersal = 0.11f;

  rendermodel_metallic = 0.1f;
  rendermodel_roughness = 0.05f;
  rendermodel_ambient = 0.8f; // bug open scene
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

    }
  }
     
  if (show_controllers_window && gui::Begin("Controllers", &show_controllers_window)) {
    for (int i = 0;i < 2;++i) {
      if (controller_mask & (1 << i)) {
        gui::Text("controller %d", i);
        vr::VRControllerState_t controller_state;
        vr::VRSystem()->GetControllerState(controller_device[i], &controller_state);
  
        static const std::map<const char *, vr::EVRButtonId> buttons{ 
          { "grip", vr::k_EButton_Grip },
          { "menu", vr::k_EButton_ApplicationMenu }          
        };

        for (auto b : buttons) {
          auto button_mask = vr::ButtonMaskFromId(b.second);
          bool touched = (controller_state.ulButtonTouched & button_mask) != 0;
          bool pressed = (controller_state.ulButtonPressed & button_mask) != 0;
          if (touched || pressed) {
            gui::BulletText("%s%s%s",
              b.first,
              touched ? " (touched)" : "",
              pressed ? " (pressed)" : ""
            );
          }
        }
          
        for (int a = 0;a < vr::k_unControllerStateAxisCount; ++a) {
          auto prop = vr::ETrackedDeviceProperty(int(vr::Prop_Axis0Type_Int32) + a);
          auto axis_type = vr::EVRControllerAxisType(vr::VRSystem()->GetInt32TrackedDeviceProperty(controller_device[i], prop));
          const char * axis_type_name = vr::VRSystem()->GetControllerAxisTypeNameFromEnum(axis_type);
          auto button_id = vr::EVRButtonId(vr::k_EButton_Axis0 + a);
          auto button_mask = vr::ButtonMaskFromId(button_id);
          bool touched = (controller_state.ulButtonTouched & button_mask) != 0;
          bool pressed = (controller_state.ulButtonPressed & button_mask) != 0;
          switch (axis_type) {
            case vr::k_eControllerAxis_Trigger:
              gui::BulletText("trigger %d: %.02f%s%s",
                a,
                controller_state.rAxis[a].x,
                touched ? " (touched)" : "",
                pressed ? " (pressed)" : ""
              );
              break;
            case vr::k_eControllerAxis_TrackPad:
            case vr::k_eControllerAxis_Joystick:
              gui::BulletText("%s %d: (%.02f,%.02f)%s%s",
                axis_type == vr::k_eControllerAxis_TrackPad ? "trackpad" : "joystick",
                a,
                controller_state.rAxis[a].x,
                controller_state.rAxis[a].y,
                touched ? " (touched)" : "",
                pressed ? " (pressed)" : ""
              );
            default: break;
          }
        }
      }
    }
    gui::End();
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

    timer::start_frame();

    l->info("gui frame");
    gui.new_frame();

    l->info("polling rendermodels");
    rendermodels.poll();

    l->info("get_poses");
    get_poses();

    l->info("quality.new_frame");
    last_resolve_buffer_usage = resolve_buffer_usage;
    quality.new_frame(vr, &render_buffer_usage, &resolve_buffer_usage);
    viewport_w = quality.viewport_w;
    viewport_h = quality.viewport_h;

    l->info("updating sky");
    sky.update(*this);

    l->info("submit_uniforms");
    submit_uniforms();


    l->info("render_stencil");
    distorted.render_stencil();

    // begin scene

    l->info("drawing rendermodels");
    rendermodels.draw(device_mask);

    l->info("skybox");
    sky.render();

    // end scene

    l->info("post");
    quality.resolve(post.presolve);
    post.process();

    if (gl_finish_hack) glFinish();
    l->info("present");
    quality.present(read_pixel_hack);

    l->info("desktop");    
    desktop_display();

    if (show_gui()) return;

    quality.swap();
    window.swap();
  }
}

void app::desktop_display() {
  static elapsed_timer timer("desktop");
  timer_block timed(timer);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_MULTISAMPLE);
  auto flags = SDL_GetWindowFlags(window.sdl_window);

  if (flags & SDL_WINDOW_MINIMIZED) {
    glFlush();
    return; // drop gui on the floor?
  }

  int w, h;
  SDL_GetWindowSize(window.sdl_window, &w, &h);

  glDisable(GL_STENCIL_TEST);
  glStencilMask(1);
  glViewport(0, 0, w, h);
  glScissor(0, 0, w, h);
  glClearColor(0.18f, 0.18f, 0.18f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


  stereo_fbo * display_fbo;
  float display_resolve_buffer_usage;
  GLsizei display_viewport_w, display_viewport_h;

  if (quality.double_buffer) {
    display_fbo = &quality.last_resolve_fbo();
    display_resolve_buffer_usage = last_resolve_buffer_usage;
    display_viewport_w = quality.last_viewport_w;
    display_viewport_h = quality.last_viewport_h;
  } else {
    display_fbo = &quality.current_resolve_fbo();
    display_resolve_buffer_usage = resolve_buffer_usage;
    display_viewport_w = quality.viewport_w;
    display_viewport_h = quality.viewport_h;
  }

  // lets find an aspect ratio preserving viewport
  switch (desktop_view) {
    case 0: break;
    case 1: {
      auto dim = fit_viewport(quality.aspect_ratio * 2, w, h);
      glViewport(dim.x, dim.y, dim.w, dim.h);
      distorted.render(3, display_fbo->texture_handle, display_resolve_buffer_usage);
      break;
    }
    case 2:
    case 3: {
      auto dim = fit_viewport(quality.aspect_ratio, w, h);
      glViewport(dim.x - (desktop_view - 2) * dim.w, dim.y, dim.w * 2, dim.h);
      glScissor(dim.x, dim.y, dim.w, dim.h);
      glEnable(GL_SCISSOR_TEST);
      distorted.render(desktop_view - 1, display_fbo->texture_handle, display_resolve_buffer_usage);
      glDisable(GL_SCISSOR_TEST);
      break;
    }
    case 4: {
      auto dim = fit_viewport(quality.aspect_ratio * 2, w, h);
      glBlitNamedFramebuffer(
        display_fbo->fbo_view[0], 0,
        0, 0,
        display_viewport_w, display_viewport_h,
        dim.x, dim.y,
        dim.x + dim.w / 2, dim.y + dim.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
      glBlitNamedFramebuffer(
        display_fbo->fbo_view[1], 0,
        0, 0,
        display_viewport_w, display_viewport_h,
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
        display_fbo->fbo_view[0], 0,
        0, 0,
        display_viewport_w, display_viewport_h,
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
        display_fbo->fbo_view[1], 0,
        0, 0,
        display_viewport_w, display_viewport_h,
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
      gui::MenuItem("Controllers", nullptr, &show_controllers_window);
      gui::EndMenu();
    }

    gui::EndMainMenuBar();
  }

  if (show_settings_window) {
    gui::Begin("Settings", &show_settings_window);    
    gui::SliderInt("desktop view", &desktop_view, 0, int(countof(desktop_views) - 1));
    gui::Checkbox("double buffer resolve", &quality.double_buffer); 
    gui::SameLine();
    gui::Checkbox("finish", &gl_finish_hack);
    gui::SameLine();
    gui::Checkbox("read pixel", &read_pixel_hack);
    gui::Text("%s", desktop_views[desktop_view]);
    bool seascape = enable_seascape; gui::Checkbox("seascape", &seascape); enable_seascape = seascape;
    gui::SliderFloat("exposure", &exposure, -30, 30);
    gui::SliderFloat("bloom exposure", &bloom_exposure, -30, 30);
    gui::SliderFloat("bloom magnitude", &bloom_magnitude, 0, 10);
    gui::SliderFloat("blur sigma", &blur_sigma, 0, 10);
    gui::SliderFloat("flare exposure", &lens_flare_exposure, -30, 30);
    bool lensflare = use_lens_flare; gui::Checkbox("lens flares", &lensflare); use_lens_flare = lensflare;
    gui::SliderFloat("flare halo radius", &lens_flare_halo_radius, -2, 2);
    gui::SliderInt("flare ghosts", &lens_flare_ghosts, 0, 20);
    gui::SliderFloat("flare ghost dispersal", &lens_flare_ghost_dispersal, 0.001f, 2.f);
    gui::End();
  }

  if (show_rendermodel_window) {
    gui::Begin("Rendermodels", &show_rendermodel_window);
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
  logging::harness logs("vr", "gl", "al", "main", "app", "post", "distortion", "rendermodel");
  SetProcessDPIAware(); // lest SDL2 lie and always tell us that DPI = 96
  cds_main_thread_attachment<> main_thread; // Allow use of concurrent data structures in the main threads
  log("main")->info("pid: {}", GetCurrentProcessId());
  if (_wchdir(executable_path().parent_path().parent_path().parent_path().native().c_str()))
    log("main")->warn("unable to set working directory");
  app main;
  main.run(); 
  return 0;
}