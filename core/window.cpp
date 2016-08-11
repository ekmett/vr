#include <stdio.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include "window.h"
#include "util.h"
#include "log.h"

using namespace std;
using namespace spdlog;

namespace core {
  sdl_window::sdl_window(shared_ptr<logger> & log, string title, bool debug, int windowX, int windowY, int width, int height)
    : core::window(width, height), log(log) {
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
      case SDL_APP_TERMINATING:
        on_app_terminating();
        return true;
      case SDL_QUIT:
        on_quit();
        return true;

        // windows
      case SDL_WINDOWEVENT: {
        uint32_t id = sdlEvent.window.windowID;
        switch (sdlEvent.window.event) {
        case SDL_WINDOWEVENT_SHOWN: on_window_shown(id); break;
        case SDL_WINDOWEVENT_HIDDEN: on_window_hidden(id); break;
        case SDL_WINDOWEVENT_CLOSE: on_window_close(id); break;
        case SDL_WINDOWEVENT_EXPOSED: on_window_exposed(id); break;
        case SDL_WINDOWEVENT_MINIMIZED: on_window_minimized(id); break;
        case SDL_WINDOWEVENT_MAXIMIZED: on_window_maximized(id); break;
        case SDL_WINDOWEVENT_RESTORED: on_window_restored(id); break;
        case SDL_WINDOWEVENT_ENTER: on_window_enter(id); break;
        case SDL_WINDOWEVENT_LEAVE: on_window_leave(id); break;
        case SDL_WINDOWEVENT_FOCUS_GAINED: on_window_focus_gained(id); break;
        case SDL_WINDOWEVENT_FOCUS_LOST: on_window_focus_lost(id); break;
        case SDL_WINDOWEVENT_SIZE_CHANGED: on_window_size_changed(id, sdlEvent.window.data1, sdlEvent.window.data2); break;
        case SDL_WINDOWEVENT_RESIZED: on_window_resize(id, sdlEvent.window.data1, sdlEvent.window.data2); break;
        default:
          log->info("unhandled SDL window event ({}) on window {}", sdlEvent.window.event, id);
          break;
        }
      }

                            // keys
      case SDL_KEYUP: on_key_up(sdlEvent.key); break;
      case SDL_KEYDOWN:
        on_key_down(sdlEvent.key);
        if (sdlEvent.key.keysym.sym == SDLK_ESCAPE || sdlEvent.key.keysym.sym == SDLK_q)
          return true;
        break;

        // mouse
      case SDL_MOUSEBUTTONDOWN: on_mouse_button_down(sdlEvent.button); break;
      case SDL_MOUSEBUTTONUP: on_mouse_button_up(sdlEvent.button); break;
      case SDL_MOUSEMOTION: on_mouse_motion(sdlEvent.motion); break;
      case SDL_MOUSEWHEEL: on_mouse_wheel(sdlEvent.wheel); break;

        // joysticks
      case SDL_JOYBUTTONUP: on_joy_button_up(sdlEvent.jbutton); break;
      case SDL_JOYBUTTONDOWN: on_joy_button_down(sdlEvent.jbutton); break;
      case SDL_JOYAXISMOTION: on_joy_axis_motion(sdlEvent.jaxis); break;
      case SDL_JOYBALLMOTION: on_joy_ball_motion(sdlEvent.jball); break;
      case SDL_JOYDEVICEADDED: on_joy_device_added(sdlEvent.jdevice.which); break;
      case SDL_JOYDEVICEREMOVED: on_joy_device_removed(sdlEvent.jdevice.which); break;
      case SDL_JOYHATMOTION: on_joy_hat_motion(sdlEvent.jhat); break;
      case SDL_CONTROLLERAXISMOTION: on_controller_axis_motion(sdlEvent.caxis); break;
      case SDL_CONTROLLERBUTTONDOWN: on_controller_button_down(sdlEvent.cbutton); break;
      case SDL_CONTROLLERBUTTONUP: on_controller_button_up(sdlEvent.cbutton); break;
      case SDL_CONTROLLERDEVICEADDED: on_controller_device_added(sdlEvent.cdevice); break;
      case SDL_CONTROLLERDEVICEREMOVED: on_controller_device_removed(sdlEvent.cdevice); break;
      case SDL_CONTROLLERDEVICEREMAPPED: on_controller_device_remapped(sdlEvent.cdevice); break;

        // text
      case SDL_TEXTEDITING: on_text_editing(sdlEvent.edit); break;
      case SDL_TEXTINPUT: on_text_input(sdlEvent.text); break;

        // audio devices
      case SDL_AUDIODEVICEADDED: on_audio_device_added(sdlEvent.adevice); break;
      case SDL_AUDIODEVICEREMOVED: on_audio_device_removed(sdlEvent.adevice); break;

        // files
      case SDL_DROPFILE: {
        wstring filename = from_utf8(sdlEvent.drop.file);
        on_drop_file(filename);
        SDL_free(sdlEvent.drop.file);
        break;
      }


      default:
        log->info("unsupported SDL event: {}", show_sdl_event(sdlEvent.type));
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

}