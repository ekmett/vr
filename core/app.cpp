#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <gl/glu.h>
#include <openvr.h>
#include <string>
#include <Windows.h>
#include <ShlObj.h>
#include <glm/gtc/type_ptr.hpp>

#define SPDLOG_WCHAR_FILENAMES
#include <spdlog/spdlog.h>

#include "app.h"
#include "gl_util.h"

using namespace vr;
using namespace std;
using namespace spdlog;

static const bool debugOpenGL = true;
static const bool vblank = false;
static const string windowTitle = "core";
static const float nearZ = 0.1f;
static const float farZ = 30.0f;

inline spdlog::level::level_enum gl_log_severity(GLenum severity) {
  using namespace spdlog::level;
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH: return critical;
  case GL_DEBUG_SEVERITY_MEDIUM: return warn; 
  default: return info;
  }
}

void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {  
  ((logger*)userParam)->log(gl_log_severity(severity), "{}: {}: {}: {}", gl_source(source), gl_message_type(type), id, message);
}

void die(const char * title, const char *fmt, ...) {
	va_list args;
	char buffer[2048];

	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, buffer, NULL);
  exit(1);
}

static inline glm::mat4 hmd_mat4(const vr::HmdMatrix44_t & m) {
  return glm::make_mat4((float*)&m.m);
}

static inline glm::mat3x4 hmd_mat3x4(const vr::HmdMatrix34_t & m) {
  return glm::make_mat3x4((float*)&m.m);
}

static inline vr::EVREye vr_eye(int i) {
  return (i == 0) ? vr::Eye_Left : vr::Eye_Right;
}

std::string GetTrackedDeviceString(vr::IVRSystem *hmd, vr::TrackedDeviceIndex_t device, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *error = NULL) {
  uint32_t unRequiredBufferLen = hmd->GetStringTrackedDeviceProperty(device, prop, NULL, 0, error);
  if (unRequiredBufferLen == 0)
    return "";

  char *pchBuffer = new char[unRequiredBufferLen];
  unRequiredBufferLen = hmd->GetStringTrackedDeviceProperty(device, prop, pchBuffer, unRequiredBufferLen, error);
  std::string sResult = pchBuffer;
  delete[] pchBuffer;
  return sResult;
}

const char * label_eye(int i) {
  switch (i) {
  case 0: return "left";
  case 1: return "right";
  default: return "unknown";
  }
}

int app::run(int argc, char ** argv) {
  if (!VR_IsHmdPresent()) {
    printf("No head mounted device detected.\n");
    exit(1);
  }

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    printf("Unable to initialize SDL.\n%s\n", SDL_GetError());
    exit(1);
  }

  // Initialize OpenVR
  auto eError = VRInitError_None;
  auto hmd = VR_Init(&eError, VRApplication_Scene);

  if (eError != VRInitError_None) {
    hmd = nullptr;
    die(__FUNCTION__, "Unable to initialize OpenVR.\n%s", VR_GetVRInitErrorAsEnglishDescription(eError));
  }

  auto renderModels = (IVRRenderModels *)VR_GetGenericInterface(IVRRenderModels_Version, &eError);
  if (!renderModels) {
    hmd = nullptr;
    VR_Shutdown();
    die(__FUNCTION__, "Unable to get render model interface.\n%s", VR_GetVRInitErrorAsEnglishDescription(eError));
  }

  int windowX = 700, windowY = 100, windowWidth = 1280, windowHeight = 720;

  Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

  if (debugOpenGL)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

  // create the sdl window
  auto window = SDL_CreateWindow(windowTitle.c_str(), windowX, windowY, windowWidth, windowHeight, windowFlags);
  if (window == nullptr)
    die(__FUNCTION__, "Window could not be created.\n%s\n", SDL_GetError());

  // create the gl context
  auto context = SDL_GL_CreateContext(window);
  if (context == nullptr)
    die(__FUNCTION__, "OpenGL context could not be created.\n%s\n", SDL_GetError());

  // initialize glew
  glewExperimental = GL_TRUE;
  auto nGlewError = glewInit();
  if (nGlewError != GLEW_OK)
    die(__FUNCTION__, "Error initializing GLEW.\n%s\n", (const char *)glewGetErrorString(nGlewError));
  glGetError(); // to clear the error caused deep in GLEW

  if (SDL_GL_SetSwapInterval(vblank ? 1 : 0) < 0)
    die(__FUNCTION__, "Unable to set VSync.\n%s\n", SDL_GetError());

  // show driver information in the window title
  auto driver = GetTrackedDeviceString(hmd, k_unTrackedDeviceIndex_Hmd, Prop_TrackingSystemName_String);
  auto display = GetTrackedDeviceString(hmd, k_unTrackedDeviceIndex_Hmd, Prop_SerialNumber_String);
  auto windowTitleWithDriver = windowTitle + " " + driver + " " + display;
  SDL_SetWindowTitle(window, windowTitleWithDriver.c_str());

  shared_ptr<logger> gl_log;
  if (debugOpenGL) {
    // get appdata folder
    PWSTR userAppData = nullptr;
    auto appDataOk = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &userAppData);
    if (appDataOk != S_OK)
      die(__FUNCTION__, "Unable to locate application data folder");
    auto localAppData = wstring(userAppData) + L"\\core";
    _wmkdir(localAppData.c_str()); // freely allow this to fail if it is already there.
    gl_log = basic_logger_mt("gl", localAppData + L"\\gl.log");
    glDebugMessageCallback((GLDEBUGPROC)debugCallback, gl_log._Get());
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_LOW, 5, "start");
  }

  // how big does the hmd want the render target to be, anyways?
  uint32_t renderWidth, renderHeight;
  hmd->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

  // allocate eyes
  struct eyes eyes;

  for (int i=0;i<2;++i) {
    auto e = vr_eye(i);
    eyes.projection[i] = hmd_mat4(hmd->GetProjectionMatrix(e, nearZ, farZ, API_OpenGL));
    eyes.pose[i] = hmd_mat3x4(hmd->GetEyeToHeadTransform(e));
  }

  glGenFramebuffers(2, eyes.renderFramebufferId);
  glGenRenderbuffers(2, eyes.depthBufferId);
  glGenTextures(2, eyes.renderTextureId);
  glGenFramebuffers(2, eyes.resolveFramebufferId);
  glGenTextures(2, eyes.resolveTextureId);

  // build eyes so we can see
  for (int i = 0;i < 2;++i) {
    const char * l = label_eye(i);
    glBindFramebuffer(GL_FRAMEBUFFER, eyes.renderFramebufferId[i]);
    objectLabelf(GL_FRAMEBUFFER, eyes.renderFramebufferId[i], "%s eye render frame buffer", l);
    glBindRenderbuffer(GL_RENDERBUFFER, eyes.depthBufferId[i]);
    objectLabelf(GL_RENDERBUFFER, eyes.depthBufferId[i], "%s eye depth buffer", l);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, renderWidth, renderHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, eyes.depthBufferId[i]);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, eyes.renderTextureId[i]);
    objectLabelf(GL_TEXTURE, eyes.renderTextureId[i], "%s eye render texture", l);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, renderWidth, renderHeight, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, eyes.renderTextureId[i], 0);
    glBindFramebuffer(GL_FRAMEBUFFER, eyes.resolveFramebufferId[i]);
    objectLabelf(GL_FRAMEBUFFER, eyes.resolveFramebufferId[i], "%s eye resolve frame buffer", l);
    glBindTexture(GL_TEXTURE_2D, eyes.resolveTextureId[i]);
    objectLabelf(GL_TEXTURE, eyes.resolveTextureId[i], "%s eye resolve texture", l);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, renderWidth, renderHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eyes.resolveTextureId[i], 0);
  } 

  // check FBO status
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
    die(__FUNCTION__, "Unable to construct your eyes");

  // let it go
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // run the application
  app game(*hmd, *renderModels, windowWidth, windowHeight, window, eyes);

  return 0; // game.play();
}

app::~app() {
  // gouge out your eyes
  glDeleteRenderbuffers(2, eyes.depthBufferId);
  glDeleteTextures(2, eyes.renderTextureId);
  glDeleteFramebuffers(2, eyes.renderFramebufferId);
  glDeleteTextures(2, eyes.resolveTextureId);
  glDeleteFramebuffers(2, eyes.resolveFramebufferId);

  if (debugOpenGL) {   
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_LOW, 4, "stop");
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
    glDebugMessageCallback(nullptr, nullptr);
    auto gl_log = spdlog::get("gl");
    if (gl_log != nullptr)
      gl_log->flush();        
  }

  VR_Shutdown();
  SDL_DestroyWindow(window);
  SDL_Quit();
}