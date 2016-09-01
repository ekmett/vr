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

using namespace framework;
using namespace filesystem;

static const float super_sampling_factor = 1.0;
static const int msaa_samples = 8;

// used reversed [1..0] floating point z rather than the classic [-1..1] mapping
#define USE_REVERSED_Z

struct app {
  app();
  ~app();

  void run();
  bool show_gui(bool * open = nullptr);
  void tonemap();
  void present();

  sdl::window window; // must come before anything that needs opengl support in this object
  gl::compiler compiler;
  openvr::system vr;
  sky sky;
  gui::system gui;
  //overlay dashboard;
  vr::TrackedDevicePose_t physical_pose[vr::k_unMaxTrackedDeviceCount]; // current poses
  vr::TrackedDevicePose_t predicted_pose[vr::k_unMaxTrackedDeviceCount]; // poses 2 frames out
  controllers controllers;
  std::mt19937 rng; // for the main thread  
  vr::IVRCompositor & compositor;
  distortion distorted;
  openal::system al;
  int desktop_view;
  bool skybox_visible = true;

  struct {
    uint32_t w, h;
    union {
      GLuint fbo[2];
      struct {
        GLuint render_fbo, resolve_fbo;
      };
    };
    GLuint depth;  // renderbuffer for render framebuffer
    union {
      GLuint texture[2];
      struct {
        GLuint render_texture, resolve_texture;
      };
    };
  } display;

  mat4 eyeProjectionMatrix[2];
  mat4 eyePoseMatrix[2];
  float nearClip, farClip;
};

#ifdef USE_REVERSED_Z
static float reverseZ_contents[16] = {
   1.f, 0.f, 0.f, 0.f,
   0.f, 1.f, 0.f, 0.f,
   0.f, 0.f, -1.f, 1.f,
   0.f, 0.f, 1.f, 0.f };

static mat4 reverseZ = glm::make_mat4(reverseZ_contents);
#endif

app::app() 
  : window("proc", { 4, 5, gl::profile::core }, true,50, 50, 1280,1024)
  , compiler(path("shaders"))
  , vr()
  // ,  dashboard("proc","Debug",1024,1024),
  , compositor(*vr::VRCompositor())
  , nearClip(0.1f)
  , farClip(10000.f)
  , gui(window)
  , distorted()
  , desktop_view(1)
  , sky(vec3(0.0,0.05,0.9), 2.0_degrees, vec3(0.1,0.1,0.3), 2.f) {

  // load matrices.
  for (int i = 0;i < 2;++i) {
#ifdef USE_REVERSED_Z
    eyeProjectionMatrix[i] = reverseZ * openvr::hmd_mat4(vr.handle->GetProjectionMatrix(vr::EVREye(i), nearClip, farClip, vr::API_DirectX));
#else
    eyeProjectionMatrix[i] = openvr::hmd_mat4(vr.handle->GetProjectionMatrix(vr::EVREye(i), nearClip, farClip, vr::API_OpenGL));
#endif
    eyePoseMatrix[i] = openvr::hmd_mat3x4(vr.handle->GetEyeToHeadTransform(vr::EVREye(i)));
  }

#ifdef USE_REVERSED_Z
  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
  glDepthFunc(GL_GREATER);
  glClearDepth(0.f);
#endif

  // set up rendering targets  
  vr.handle->GetRecommendedRenderTargetSize(&display.w, &display.h);
  display.w *= super_sampling_factor;
  display.h *= super_sampling_factor;
  glCreateFramebuffers(countof(display.fbo), display.fbo);
  gl::label(GL_FRAMEBUFFER, display.render_fbo, "render fbo");
  gl::label(GL_FRAMEBUFFER, display.resolve_fbo, "resolve fbo");

  glCreateRenderbuffers(1, &display.depth);
  gl::label(GL_RENDERBUFFER, display.depth, "render depth");
  glNamedRenderbufferStorageMultisample(display.depth, msaa_samples, GL_DEPTH32F_STENCIL8, display.w * 2, display.h); // ask for a floating point z buffer
  glNamedFramebufferRenderbuffer(display.render_fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, display.depth);

  glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &display.render_texture);
  gl::label(GL_TEXTURE, display.render_texture, "render texture");
  glTextureImage2DMultisampleNV(display.render_texture, GL_TEXTURE_2D_MULTISAMPLE, msaa_samples, GL_RGBA16F, display.w * 2, display.h, true);
  glNamedFramebufferTexture(display.render_fbo, GL_COLOR_ATTACHMENT0, display.render_texture, 0);
 

  glCreateTextures(GL_TEXTURE_2D, 1, &display.resolve_texture);
  gl::label(GL_TEXTURE, display.resolve_texture, "resolve texture");
  glTextureParameteri(display.resolve_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(display.resolve_texture, GL_TEXTURE_MAX_LEVEL, 0);
  glTextureImage2DEXT(display.resolve_texture, GL_TEXTURE_2D, 0, GL_RGBA8, display.w * 2, display.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glNamedFramebufferTexture(display.resolve_fbo, GL_COLOR_ATTACHMENT0, display.resolve_texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    die("Unable to allocate frame buffer");

  SDL_DisplayMode wdm;
  SDL_GetWindowDisplayMode(window.sdl_window, &wdm);
  log("app")->info("desktop window refresh rate: {}hz", wdm.refresh_rate); // TODO: compute frame skipping for gui and desktop display off this relative to the gpu rate.

  SDL_StartTextInput();
}

app::~app() {
  SDL_StopTextInput();

  glDeleteFramebuffers(countof(display.fbo), display.fbo);
  glDeleteRenderbuffers(1, &display.depth);
  glDeleteTextures(countof(display.texture), display.texture);
}

struct viewport_dim {
  GLint x, y, w, h;
};

viewport_dim fit_viewport(float aspectRatio, int w, int h) {
  GLint x = GLint(h * aspectRatio);
  if (x <= w) {
    return viewport_dim { (w-x)/2, 0, x, h };
  } else {
    GLint y = GLint(w / aspectRatio);
    return viewport_dim { 0, (h-y)/2, w, y };
  }
}

void app::tonemap() {
  // time to downsample into the bloom filter 

  // copy the msaa render target to a lower quality 'resolve' texture for display
  glBlitNamedFramebuffer(display.render_fbo, display.resolve_fbo, 0, 0, display.w * 2, display.h, 0, 0, display.w * 2, display.h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}
void app::present() {
  vr::Texture_t eyeTexture{ (void*)(intptr_t)(display.resolve_texture), vr::API_OpenGL, vr::ColorSpace_Gamma };
  for (int i = 0;i < 2;++i) {
    vr::VRTextureBounds_t eyeBounds = { 0.5f * i, 0, 0.5f + 0.5f * i , 1 };
    vr::VRCompositor()->Submit(vr::EVREye(i), &eyeTexture, &eyeBounds);
  }
  // let the compositor know we handed off a frame
  compositor.PostPresentHandoff();
}

void app::run() {
  while (!vr.poll() && !window.poll()) {
    // clear the display window  
    glClearColor(0.0f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // start a new imgui frame
    gui.new_frame();

    glEnable(GL_MULTISAMPLE);

    // switch to the hmd
    glBindFramebuffer(GL_FRAMEBUFFER, display.render_fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, display.w * 2, display.h);
    distorted.render_stencil();

    glViewportIndexedf(0, 0, 0, (float)display.w, (float)display.h);                // viewport 0 left eye
    glViewportIndexedf(1, (float)display.w, 0, (float)display.w, (float)display.h); // viewport 1 right eye
    
    // compute stuff that can deal with approximate pose info, such as most of the shadow maps here
    compositor.WaitGetPoses(physical_pose, vr::k_unMaxTrackedDeviceCount, predicted_pose, 0);    
    mat4 hmdToWorld = openvr::hmd_mat3x4(physical_pose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
    mat4 model_view[2];
    mat4 eyes[2];
    for (int i = 0;i < 2;++i) {
      model_view[i] = inverse(hmdToWorld * eyePoseMatrix[i]);
      eyes[i] = eyeProjectionMatrix[i] * model_view[i];
    }

    if (skybox_visible)
      sky.render(eyeProjectionMatrix, model_view);
    controllers.render(physical_pose, eyes); // draw controller rays

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

      // lets find an aspect ratio preserving viewport
      switch (desktop_view) {
        case 0: break;
        case 1: {
          auto dim = fit_viewport(float(display.w) * 2 / display.h, w, h);
          glViewport(dim.x, dim.y, dim.w, dim.h);
          distorted.render(display.resolve_texture);
          break; // paint over the entire sdl window
        }
        case 2: 
        case 3: {
          auto dim = fit_viewport(float(display.w) / display.h, w, h);
          glViewport(dim.x - (desktop_view - 2) * dim.w, dim.y, dim.w * 2, dim.h);
          glScissor(dim.x, dim.y, dim.w, dim.h);
          glEnable(GL_SCISSOR_TEST);
          distorted.render(display.resolve_texture);
          glDisable(GL_SCISSOR_TEST);
          break;
        }
        case 4: {
          auto dim = fit_viewport(float(display.w) * 2 / display.h, w, h);
          glBlitNamedFramebuffer(
            display.resolve_fbo, 0,
            0, 0,
            display.w*2, display.h,
            dim.x, dim.y,
            dim.x + dim.w, dim.y + dim.h,
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR
          );
          break;
        }
        case 5: 
        case 6: {
          auto dim = fit_viewport(float(display.w) / display.h, w, h);
          auto display_shift = (desktop_view - 5) * display.w;
          glBlitNamedFramebuffer(
            display.resolve_fbo, 0, 
            display_shift, 0, 
            display_shift + display.w, display.h, 
            dim.x, dim.y, 
            dim.x + dim.w, dim.y + dim.h, 
            GL_COLOR_BUFFER_BIT, 
            GL_LINEAR
          );
          break;
        }
      }
    }

    if (show_gui()) return;

    window.swap();
  }
}

bool app::show_gui(bool * open) {

  static bool show_debug_window = false, show_demo_window = false;
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
    gui::Text("Hello World");
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