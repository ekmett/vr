#include "stdafx.h"
#include <random>
#include <functional>
#include "framework/worker.h"
#include "framework/gl.h"
#include "framework/signal.h"
#include "imgui.h"
#include "imgui_impl.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace framework;
using namespace glm;

// used reversed [1..0] floating point z rather than the classic [-1..1] mapping
#define USE_REVERSED_Z

struct app {
  app();
  ~app();

  void run();

  sdl::window window; // must come before anything that needs opengl support in this object
  openvr::system vr;
  vr::TrackedDevicePose_t physical_pose[vr::k_unMaxTrackedDeviceCount]; // current poses
  vr::TrackedDevicePose_t predicted_pose[vr::k_unMaxTrackedDeviceCount]; // poses 2 frames out
  std::mt19937 rng; // for the main thread
  vr::IVRCompositor & compositor;

  enum display_pass { render, resolve };
  struct {
    uint32_t w, h;
    GLuint fbo[2]; // render, resolve
    GLuint depth;  // renderbuffer for render framebuffer
    GLuint texture[2]; // render, resolve -- render is multisample, resolve is not
  } display;
  mat4 eyeProjectionMatrix[2];
  mat4 eyePoseMatrix[2];
  float nearClip, farClip;
  connection imgui_sdl_event_connection;
};

#ifdef USE_REVERSED_Z
static float reverseZ_contents[16] = { // transposed of course
   1.f, 0.f, 0.f, 0.f,
   0.f, 1.f, 0.f, 0.f,
   0.f, 0.f, -1.f, 1.f,
   0.f, 0.f, 1.f, 0.f };

static mat4 reverseZ = glm::make_mat4(reverseZ_contents);
#endif

app::app() : window("proc", { 4, 5, gl::profile::core }, false, 100, 100, 800, 600), vr(), compositor(*vr::VRCompositor()), nearClip(0.1f), farClip(10000.f) {


  // load matrices.
  for (int i = 0;i < 2;++i) {
    // use directx [0,1] z clip plane and reverse for improvied floating point depth buffer precision
#ifdef USE_REVERSED_Z
    eyeProjectionMatrix[i] = reverseZ * openvr::hmd_mat4(vr.handle->GetProjectionMatrix(vr::EVREye(i), nearClip, farClip, vr::API_DirectX));
#else
    float l, r, t, b;
    vr.handle->GetProjectionRaw(vr::EVREye(i), &l, &r, &t, &b);
    eyeProjectionMatrix[i] = glm::frustum(l, r, b, t, nearClip, farClip);
#endif
    eyePoseMatrix[i] = openvr::hmd_mat3x4(vr.handle->GetEyeToHeadTransform(vr::EVREye(i)));
  }

  // set up rendering targets  
  vr.handle->GetRecommendedRenderTargetSize(&display.w, &display.h);
  glCreateFramebuffers(2, display.fbo);
  glGenRenderbuffers(1, &display.depth);
  glGenTextures(2, display.texture);

#ifdef USE_REVERSED_Z
  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); // switch to reversed z
  glDepthFunc(GL_GREATER);
  glClearDepth(0.f);
#endif

  // set up render fbo
  glBindFramebuffer(GL_FRAMEBUFFER, display.fbo[render]);
  glBindRenderbuffer(GL_RENDERBUFFER, display.depth);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT32F, display.w * 2, display.h); // ask for a floating point z buffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, display.depth);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, display.texture[render]);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, display.w * 2, display.h, true);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, display.texture[render], 0);

  // set up resolve fbo
  glBindFramebuffer(GL_FRAMEBUFFER, display.fbo[resolve]);
  glBindTexture(GL_TEXTURE_2D, display.texture[resolve]); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, display.w * 2, display.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, display.texture[resolve], 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    die("Unable to allocate frame buffer");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  ImGui_ImplSdlGL3_Init(window.sdl_window);
  imgui_sdl_event_connection = window.on_event.connect([](SDL_Event & event) { ImGui_ImplSdlGL3_ProcessEvent(&event); });

}

app::~app() {
  ImGui_ImplSdlGL3_Shutdown();

  glDeleteFramebuffers(2, display.fbo);
  glDeleteRenderbuffers(1, &display.depth);
  glDeleteTextures(2, display.texture);
}

void app::run() {
  while (!vr.poll() && !window.poll()) {
    // clear the display window
    glClearColor(0.15f, 0.15f, 0.15f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);

    glBindFramebuffer(GL_FRAMEBUFFER, display.fbo[render]);
    glViewportIndexedf(0, 0, 0, display.w, display.h); // viewport 0 left eye
    glViewportIndexedf(1, display.w, 0, display.w, display.h); // viewport 1 right eye
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f); // nice background color, but not black
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // compute stuff that can deal with approximate pose info, such as most of the shadow maps

    compositor.WaitGetPoses(physical_pose, vr::k_unMaxTrackedDeviceCount, predicted_pose, 0);
    
    // we now know _precisely_ where we are and we're ready to draw to other things here
    // TODO: render eye-specific stuff here

    glDisable(GL_MULTISAMPLE);
    glBlitNamedFramebuffer(display.fbo[render], display.fbo[resolve], 0, 0, display.w * 2, display.h, 0, 0, display.w * 2, display.h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    {
      vr::Texture_t eyeTexture = { reinterpret_cast<void*>(display.texture[resolve]), vr::API_OpenGL, vr::ColorSpace_Gamma };
      for (int i = 0;i < 2;++i) {
        vr::VRTextureBounds_t eyeBounds = { 0.5 * i, 0.0, 0.5 + 0.5 * i , 1.0 };
        vr::VRCompositor()->Submit(vr::EVREye(i), &eyeTexture, &eyeBounds);
      }
    }
    compositor.PostPresentHandoff();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui_ImplSdlGL3_NewFrame(window.sdl_window);
   
    if (ImGui::Button("Hello")) {
      OutputDebugStringA("Hello\n");
    }

    ImGui::Render();


    // set glViewport
    // draw anything else we need on the main SDL window, imgui elements, etc.
    
    SDL_GL_SwapWindow(window.sdl_window);
  }
}


int SDL_main(int argc, char ** argv) {
  spdlog::set_pattern("%a %b %m %Y %H:%M:%S.%e - %n %l: %v [thread %t]"); // close enough to the native notifications from openvr that the debug log is readable.
  cds_main_thread_attachment<> main_thread;

  app main;
  main.run();

  spdlog::details::registry::instance().apply_all([](shared_ptr<logger> logger) { logger->flush(); });
  spdlog::details::registry::instance().drop_all();
  return 0;
}