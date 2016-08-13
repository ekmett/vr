#pragma once
#include <stdio.h>
#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include "error.h"
#include "sdl_gl_window.h"
#include "signal.h"
#include "noncopyable.h"

#ifdef _WIN32
#pragma comment(lib, "SDL2")
#pragma comment(lib, "SDL2main")
#endif

using namespace std;
using namespace spdlog;

namespace framework {

  // re-entrant access to SDL
  struct sdl : noncopyable {    
    sdl(uint32_t flags = SDL_INIT_VIDEO);  
    virtual ~sdl();

    const uint32_t flags;

    bool poll(); // only able to be called from the video thread

    static signal<void(SDL_Event &)> on_event;
    static signal<void()> on_quit, on_app_terminating;
    static signal<void(filename_t)> on_drop_file;

    // window events
    static signal<void(uint32_t)> on_window_close, on_window_shown, on_window_hidden, on_window_exposed,
      on_window_minimized, on_window_maximized, on_window_restored,
      on_window_enter, on_window_leave, on_window_focus_gained, on_window_focus_lost;
    static signal<void(uint32_t, int, int)> on_window_resize, on_window_size_changed, on_window_moved;

    // keyboard events
    static signal<void(SDL_KeyboardEvent &)> on_key_down, on_key_up;

    // mouse events
    static signal<void(SDL_MouseMotionEvent &)> on_mouse_motion;
    static signal<void(SDL_MouseButtonEvent &)> on_mouse_button_down, on_mouse_button_up;
    static signal<void(SDL_MouseWheelEvent &)> on_mouse_wheel;

    // joystick events
    static signal<void(SDL_JoyButtonEvent &)> on_joy_button_down, on_joy_button_up;
    static signal<void(SDL_JoyAxisEvent &)> on_joy_axis_motion;
    static signal<void(SDL_JoyBallEvent &)> on_joy_ball_motion;
    static signal<void(SDL_JoyHatEvent &)> on_joy_hat_motion;
    static signal<void(uint32_t)> on_joy_device_added, on_joy_device_removed;
    static signal<void(SDL_ControllerAxisEvent &)> on_controller_axis_motion;
    static signal<void(SDL_ControllerButtonEvent &)> on_controller_button_down, on_controller_button_up;
    static signal<void(SDL_ControllerDeviceEvent &)> on_controller_device_added, on_controller_device_removed, on_controller_device_remapped;
    // text editing
    static signal<void(SDL_TextEditingEvent &)> on_text_editing;
    static signal<void(SDL_TextInputEvent &)> on_text_input;

    // audio devices
    static signal<void(SDL_AudioDeviceEvent &)> on_audio_device_added, on_audio_device_removed;

    static const char * show_event(uint32_t e);
  };
}