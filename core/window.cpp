#include <stdio.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include "window.h"
#include "util.h"
#include "log.h"

using namespace std;
using namespace spdlog;

sdl_window::sdl_window(shared_ptr<logger> & log, string title, bool debug, int windowX, int windowY, int width, int height) 
: ::window(width, height), log(log) {
  // once created OpenGL works
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    printf("Unable to initialize SDL.\n%s\n", SDL_GetError());
    exit(1);
  }

  Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

  if (debug)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

  // create the sdl window
  window = SDL_CreateWindow(title.c_str(), windowX, windowY, width, height, windowFlags);
  if (window == nullptr)
    die("Window could not be created.\n%s\n", SDL_GetError());

  // create the gl context
  context = SDL_GL_CreateContext(window);
  if (context == nullptr)
    die("OpenGL context could not be created.\n%s\n", SDL_GetError());

  // initialize glew
  glewExperimental = GL_TRUE;
  auto nGlewError = glewInit();
  if (nGlewError != GLEW_OK)
    die("Error initializing GLEW.\n%s\n", (const char *)glewGetErrorString(nGlewError));
  glGetError(); // to clear the error caused deep in GLEW

  if (SDL_GL_SetSwapInterval(0) < 0)
    die("Unable to set VSync.\n%s\n", SDL_GetError());

  SDL_StartTextInput();
  SDL_ShowCursor(SDL_DISABLE);
}

const char * show_sdl_event(uint32_t e) {
  switch (e) {
  case SDL_QUIT: return "QUIT";
  case SDL_APP_TERMINATING: return "APP_TERMINATING";
  case SDL_APP_LOWMEMORY: return "APP_LOWMEMORY";
  case SDL_APP_WILLENTERBACKGROUND: return "APP_WILLENTERBACKGROUND";
  case SDL_APP_DIDENTERBACKGROUND: return "APP_DIDENTERBACKGROUND";
  case SDL_APP_WILLENTERFOREGROUND: return "APP_WILLENTERFOREGROUND";
  case SDL_APP_DIDENTERFOREGROUND: return "APP_DIDENTERFOREGROUND";
  case SDL_WINDOWEVENT: return "WINDOWEVENT";
  case SDL_SYSWMEVENT: return "SYSWMEVENT";
  case SDL_KEYDOWN: return "KEYDOWN";
  case SDL_KEYUP: return "KEYUP";
  case SDL_TEXTEDITING: return "TEXTEDITING";
  case SDL_TEXTINPUT: return "TEXTINPUT";
  case SDL_KEYMAPCHANGED: return "KEYMAPCHANGED";
  case SDL_MOUSEMOTION: return "MOUSEMOTION";
  case SDL_MOUSEBUTTONDOWN: return "MOUSEBUTTONDOWN";
  case SDL_MOUSEBUTTONUP: return "MOUSEBUTTONUP";
  case SDL_MOUSEWHEEL: return "MOUSEWHEEL";
  case SDL_JOYAXISMOTION: return "JOYAXISMOTION";
  case SDL_JOYBALLMOTION: return "JOYBALLMOTION";
  case SDL_JOYHATMOTION: return "JOYHATMOTION";
  case SDL_JOYBUTTONDOWN: return "JOYBUTTONDOWN";
  case SDL_JOYBUTTONUP: return "JOYBUTTONUP";
  case SDL_JOYDEVICEADDED: return "JOYDEVICEADDED";
  case SDL_JOYDEVICEREMOVED: return "JOYDEVICEREMOVED";
  case SDL_CONTROLLERAXISMOTION: return "CONTROLLERAXISMOTION";
  case SDL_CONTROLLERBUTTONDOWN: return "CONTROLLERBUTTONDOWN";
  case SDL_CONTROLLERBUTTONUP: return "CONTROLLERBUTTONUP";
  case SDL_CONTROLLERDEVICEADDED: return "CONTROLLERDEVICEADDED";
  case SDL_CONTROLLERDEVICEREMOVED: return "CONTROLLERDEVICEREMOVED";
  case SDL_CONTROLLERDEVICEREMAPPED: return "CONTROLLERDEVICEREMAPPED";
  case SDL_FINGERDOWN: return "FINGERDOWN";
  case SDL_FINGERUP: return "FINGERUP";
  case SDL_FINGERMOTION: return "FINGERMOTION";
  case SDL_DOLLARGESTURE: return "DOLLARGESTURE";
  case SDL_DOLLARRECORD: return "DOLLARRECORD";
  case SDL_MULTIGESTURE: return "MULTIGESTURE";
  case SDL_CLIPBOARDUPDATE: return "CLIPBOARDUPDATE";
  case SDL_DROPFILE: return "DROPFILE";
  case SDL_AUDIODEVICEADDED: return "AUDIODEVICEADDED";
  case SDL_AUDIODEVICEREMOVED: return "AUDIODEVICEREMOVED";
  case SDL_RENDER_TARGETS_RESET: return "RENDER_TARGETS_RESET";
  case SDL_RENDER_DEVICE_RESET: return "RENDER_DEVICE_RESET";
  case SDL_USEREVENT: return "USEREVENT";
  default: return "unknown";
  }
}

bool sdl_window::poll() {
  SDL_Event sdlEvent;
  while (SDL_PollEvent(&sdlEvent) != 0) {
    switch (sdlEvent.type) {
    case SDL_QUIT: 
    case SDL_APP_TERMINATING: return true;

    case SDL_WINDOWEVENT:
      switch (sdlEvent.window.event) {
      case SDL_WINDOWEVENT_SIZE_CHANGED:
      case SDL_WINDOWEVENT_RESIZED:
        width = sdlEvent.window.data1;
        height = sdlEvent.window.data2;
        break;      
      default:
        log->info("unhandled SDL window event: {}", sdlEvent.window.event);
        break;
      }

    case SDL_KEYDOWN: 
      if (sdlEvent.key.keysym.sym == SDLK_ESCAPE || sdlEvent.key.keysym.sym == SDLK_q)
        return true;
      log->info("unhandled keydown");
    
    default:
      log->info("unhandled SDL event: {}", show_sdl_event(sdlEvent.type));
      break;
    }
  }
  return false;
}

sdl_window::~sdl_window() {
  SDL_StopTextInput();
  SDL_ShowCursor(SDL_ENABLE);
  SDL_DestroyWindow(window);
  SDL_Quit();
}