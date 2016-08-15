#include "stdafx.h"
#include "sdl_system.h"
#include "utf8.h"

namespace framework {
  namespace sdl {

    static int sdl_event_filter(void * userdata, SDL_Event * e);

    static mutex sdl_init_mutex;
    static uint64_t sdl_initializations;

    static struct {
      SDL_EventFilter filter;
      void * userdata;
    } old_eventfilter;

    // these initialization options all require event handling, and hence trigger filter installation.
    static const int event_mask = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER;

    system::system(uint32_t flags) : flags(flags) {
      if (SDL_InitSubSystem(flags) < 0)
        die("Unable to initialize SDL.\n{}", SDL_GetError());

      if (flags & event_mask) {
        lock_guard<mutex> guard(sdl_init_mutex);
        if (!++sdl_initializations) {
          if (!SDL_GetEventFilter(&old_eventfilter.filter, &old_eventfilter.userdata))
            old_eventfilter = { nullptr, nullptr };
          SDL_SetEventFilter(sdl_event_filter, nullptr);
          SDL_StartTextInput(); // let text input events through
        }
      }
    }

    system::~system() {
      // atomically kill the event filter
      SDL_QuitSubSystem(flags);

      if (flags & event_mask) {
        lock_guard<mutex> guard(sdl_init_mutex);
        if (!sdl_initializations--) {
          SDL_StopTextInput();
          SDL_SetEventFilter(old_eventfilter.filter, old_eventfilter.userdata);
        }
      }
    }

    signal<void()> system::on_quit, system::on_app_terminating;
    signal<void(filename_t)> system::on_drop_file;

    // window events
    signal<void(uint32_t)> system::on_window_close, system::on_window_shown, system::on_window_hidden, system::on_window_exposed,
      system::on_window_minimized, system::on_window_maximized, system::on_window_restored,
      system::on_window_enter, system::on_window_leave, system::on_window_focus_gained, system::on_window_focus_lost;
    signal<void(uint32_t, int, int)> system::on_window_resize, system::on_window_size_changed, system::on_window_moved;

    // keyboard events
    signal<void(SDL_KeyboardEvent &)> system::on_key_down, system::on_key_up;

    // mouse events
    signal<void(SDL_MouseMotionEvent &)> system::on_mouse_motion;
    signal<void(SDL_MouseButtonEvent &)> system::on_mouse_button_down, system::on_mouse_button_up;
    signal<void(SDL_MouseWheelEvent &)> system::on_mouse_wheel;

    // joystick events
    signal<void(SDL_JoyButtonEvent &)> system::on_joy_button_down, system::on_joy_button_up;
    signal<void(SDL_JoyAxisEvent &)> system::on_joy_axis_motion;
    signal<void(SDL_JoyBallEvent &)> system::on_joy_ball_motion;
    signal<void(SDL_JoyHatEvent &)> system::on_joy_hat_motion;
    signal<void(uint32_t)> system::on_joy_device_added, system::on_joy_device_removed;
    signal<void(SDL_ControllerAxisEvent &)> system::on_controller_axis_motion;
    signal<void(SDL_ControllerButtonEvent &)> system::on_controller_button_down, system::on_controller_button_up;
    signal<void(SDL_ControllerDeviceEvent &)> system::on_controller_device_added, system::on_controller_device_removed, system::on_controller_device_remapped;
    // text editing
    signal<void(SDL_TextEditingEvent &)> system::on_text_editing;
    signal<void(SDL_TextInputEvent &)> system::on_text_input;

    // audio devices
    signal<void(SDL_AudioDeviceEvent &)> system::on_audio_device_added, system::on_audio_device_removed;
    signal<void(SDL_Event &)> system::on_event;

    static int sdl_event_filter(void * userdata, SDL_Event * e) {
      if (!e || !system::on_event.empty()) return 1;
      switch (e->type) {
        // explicitly hooked
        case SDL_QUIT:
        case SDL_APP_TERMINATING:
        case SDL_KEYDOWN:
        case SDL_DROPFILE: return 1; // we need to clean up the filename, not sure what happens if we ignore it here

        // check if we're listening:
        case SDL_WINDOWEVENT: {
          switch (e->window.event) {
            case SDL_WINDOWEVENT_SHOWN: return !system::on_window_shown.empty();
            case SDL_WINDOWEVENT_HIDDEN: return !system::on_window_hidden.empty();
            case SDL_WINDOWEVENT_CLOSE: return !system::on_window_close.empty();
            case SDL_WINDOWEVENT_EXPOSED: return !system::on_window_exposed.empty();
            case SDL_WINDOWEVENT_MINIMIZED: return !system::on_window_minimized.empty();
            case SDL_WINDOWEVENT_MAXIMIZED: return !system::on_window_maximized.empty();
            case SDL_WINDOWEVENT_RESTORED: return !system::on_window_restored.empty();
            case SDL_WINDOWEVENT_ENTER: return !system::on_window_enter.empty();
            case SDL_WINDOWEVENT_LEAVE: return !system::on_window_leave.empty();
            case SDL_WINDOWEVENT_FOCUS_GAINED: return !system::on_window_focus_gained.empty();
            case SDL_WINDOWEVENT_FOCUS_LOST: return !system::on_window_focus_lost.empty();
            case SDL_WINDOWEVENT_SIZE_CHANGED: return !system::on_window_size_changed.empty();
            case SDL_WINDOWEVENT_RESIZED: return !system::on_window_resize.empty();
            default: return 1;
          }
        case SDL_KEYUP: return !system::on_key_up.empty();
        case SDL_MOUSEBUTTONDOWN: return !system::on_mouse_button_down.empty();
        case SDL_MOUSEBUTTONUP: return !system::on_mouse_button_up.empty();
        case SDL_MOUSEMOTION: return !system::on_mouse_motion.empty();
        case SDL_MOUSEWHEEL: return !system::on_mouse_wheel.empty();

          // joysticks
        case SDL_JOYBUTTONUP: return !system::on_joy_button_up.empty();
        case SDL_JOYBUTTONDOWN: return !system::on_joy_button_down.empty();
        case SDL_JOYAXISMOTION: return !system::on_joy_axis_motion.empty();
        case SDL_JOYBALLMOTION: return !system::on_joy_ball_motion.empty();
        case SDL_JOYDEVICEADDED: return !system::on_joy_device_added.empty();
        case SDL_JOYDEVICEREMOVED: return !system::on_joy_device_removed.empty();
        case SDL_JOYHATMOTION: return !system::on_joy_hat_motion.empty();
        case SDL_CONTROLLERAXISMOTION: return !system::on_controller_axis_motion.empty();
        case SDL_CONTROLLERBUTTONDOWN: return !system::on_controller_button_down.empty();
        case SDL_CONTROLLERBUTTONUP: return !system::on_controller_button_up.empty();
        case SDL_CONTROLLERDEVICEADDED: return !system::on_controller_device_added.empty();
        case SDL_CONTROLLERDEVICEREMOVED: return !system::on_controller_device_removed.empty();
        case SDL_CONTROLLERDEVICEREMAPPED: return !system::on_controller_device_remapped.empty();

          // text
        case SDL_TEXTEDITING: return !system::on_text_editing.empty();
        case SDL_TEXTINPUT: return !system::on_text_input.empty();

          // audio devices
        case SDL_AUDIODEVICEADDED: return !system::on_audio_device_added.empty();
        case SDL_AUDIODEVICEREMOVED: return !system::on_audio_device_removed.empty();

          // don't know, don't care, maybe should log
        default:
          log("sdl")->warn("Filtering unhandled event: {} ({})", system::show_event(e->type), e->type);
          return 0;
        }
      }
    }

    // code changes here must percolate up into sdl_event_filter!
    bool system::poll() {
      SDL_Event sdlEvent;
      while (SDL_PollEvent(&sdlEvent) != 0) {
        switch (sdlEvent.type) {
          case SDL_APP_TERMINATING: on_app_terminating(); return true;
          case SDL_QUIT: on_quit(); return true;

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
              default: break;
            }
          }

                                // keys
          case SDL_KEYUP: on_key_up(sdlEvent.key); break;
          case SDL_KEYDOWN:
            on_key_down(sdlEvent.key);
            // global hooks, remove?
            if (sdlEvent.key.keysym.sym == SDLK_ESCAPE || sdlEvent.key.keysym.sym == SDLK_q) return true;
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

            // files, note: ignoring this event would leak the filename!
          case SDL_DROPFILE: {
            wstring filename = from_utf8(sdlEvent.drop.file);
            on_drop_file(filename);
            SDL_free(sdlEvent.drop.file);
            break;
          }
          default: break;
        }
      }
      return false;
    }


    const char * system::show_event(uint32_t e) {
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
  }
}