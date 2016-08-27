#include "stdafx.h"
#include <random>
#include <functional>
#include "framework/worker.h"
#include "framework/gl.h"
#include "framework/signal.h"
#include "gui.h"
#include <glm/gtc/matrix_transform.hpp>
#include "openal.h"
#include "distortion.h"

using namespace framework;
using namespace glm;

// used reversed [1..0] floating point z rather than the classic [-1..1] mapping
// #define USE_REVERSED_Z

struct app {
  app();
  ~app();

  void run();

  sdl::window window; // must come before anything that needs opengl support in this object
  openvr::system vr;
  gui::system gui;
  openal::system al;
  vr::TrackedDevicePose_t physical_pose[vr::k_unMaxTrackedDeviceCount]; // current poses
  vr::TrackedDevicePose_t predicted_pose[vr::k_unMaxTrackedDeviceCount]; // poses 2 frames out
  std::mt19937 rng; // for the main thread
  vr::IVRCompositor & compositor;
  distortion distorted;
  
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

app::app() : window("proc", { 4, 5, gl::profile::core }, true), vr(), compositor(*vr::VRCompositor()), nearClip(0.1f), farClip(10000.f), gui(window), distorted() {

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

#ifdef USE_REVERSED_Z
  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); // switch to reversed z
  glDepthFunc(GL_GREATER);
  glClearDepth(0.f);
#endif

  // set up rendering targets  
  vr.handle->GetRecommendedRenderTargetSize(&display.w, &display.h);
  glCreateFramebuffers(2, display.fbo);
  gl::label(GL_FRAMEBUFFER, display.render_fbo, "render fbo");
  gl::label(GL_FRAMEBUFFER, display.resolve_fbo, "resolve fbo");

  glCreateRenderbuffers(1, &display.depth);
  gl::label(GL_RENDERBUFFER, display.depth, "render depth");
  glNamedRenderbufferStorageMultisample(display.depth, 4, GL_DEPTH_COMPONENT32F, display.w * 2, display.h); // ask for a floating point z buffer -- TODO add stencil?
  glNamedFramebufferRenderbuffer(display.render_fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, display.depth);

  glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &display.render_texture);
  gl::label(GL_TEXTURE, display.render_texture, "render texture");
  glTextureImage2DMultisampleNV(display.render_texture, GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, display.w * 2, display.h, true);
  glNamedFramebufferTexture(display.render_fbo, GL_COLOR_ATTACHMENT0, display.render_texture, 0);
 
  glCreateTextures(GL_TEXTURE_2D, 1, &display.resolve_texture);
  gl::label(GL_TEXTURE, display.resolve_texture, "resolve texture");
  glTextureParameteri(display.resolve_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(display.resolve_texture, GL_TEXTURE_MAX_LEVEL, 0);
  glTextureImage2DEXT(display.resolve_texture, GL_TEXTURE_2D, 0, GL_RGBA8, display.w * 2, display.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glNamedFramebufferTexture(display.resolve_fbo, GL_COLOR_ATTACHMENT0, display.resolve_texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    die("Unable to allocate frame buffer");


  SDL_StartTextInput();
}

app::~app() {
  SDL_StopTextInput();

  glDeleteFramebuffers(2, display.fbo);
  glDeleteRenderbuffers(1, &display.depth);
  glDeleteTextures(2, display.texture);
}

void app::run() {
  while (!vr.poll() && !window.poll()) {
    // clear the display window  
    glClearColor(0.15f, 0.15f, 0.45f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // start a new imgui frame
    gui.new_frame();

    glEnable(GL_MULTISAMPLE);

    // switch to the hmd
    glBindFramebuffer(GL_FRAMEBUFFER, display.render_fbo);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f); // nice background color, not black, distinct from desktop clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glViewportIndexedf(0, 0, 0, display.w, display.h);         // viewport 0 left eye
    //glViewportIndexedf(1, display.w, 0, display.w, display.h); // viewport 1 right eye
    
    // compute stuff that can deal with approximate pose info, such as most of the shadow maps

    compositor.WaitGetPoses(physical_pose, vr::k_unMaxTrackedDeviceCount, predicted_pose, 0);
    
    // we now know _precisely_ where we are and we're ready to draw to other things here
    // TODO: render eye-specific stuff here

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_MULTISAMPLE);

    // copy the msaa render target to a lower quality 'resolve' texture for display
    glBlitNamedFramebuffer(display.render_fbo, display.resolve_fbo, 0, 0, display.w * 2, display.h, 0, 0, display.w * 2, display.h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    {
      vr::Texture_t eyeTexture = { reinterpret_cast<void*>(display.resolve_texture), vr::API_OpenGL, vr::ColorSpace_Gamma };
      for (int i = 0;i < 2;++i) {
        vr::VRTextureBounds_t eyeBounds = { 0.5 * i, 0.0, 0.5 + 0.5 * i , 1.0 };
        vr::VRCompositor()->Submit(vr::EVREye(i), &eyeTexture, &eyeBounds);
      }
    }

    // let the compositor know we handed off a frame
    compositor.PostPresentHandoff();

    //{
    //  int w, h;
    //  SDL_GetWindowSize(window.sdl_window, &w, &h);
    //  glViewport(0, 0, w, h); // paint over the entire sdl window
    //}

    distorted.render(display.resolve_texture);

    gui::Text(ICON_MD_FILE_DOWNLOAD " Download");
    gui::Text(ICON_MD_FILE_UPLOAD " Upload");

   // gui::ShowTestWindow();

    gui::Render();

    window.swap();
  }
}


int SDL_main(int argc, char ** argv) {
  SetProcessDPIAware(); // if we don't call this, then SDL2 will lie and always tell us that DPI = 96
  spdlog::set_pattern("%a %b %m %Y %H:%M:%S.%e - %n %l: %v"); // [thread %t]"); // close enough to the native notifications from openvr that the debug log is readable.
  cds_main_thread_attachment<> main_thread; // Allow use of concurrent data structures in the main threads

  app main;
  main.run();

  spdlog::details::registry::instance().apply_all([](shared_ptr<logger> logger) { logger->flush(); }); // make sure the logs are flushed before shutting down
  spdlog::details::registry::instance().drop_all(); // allow any dangling logs with no references to more gracefully shutdown
  return 0;
}