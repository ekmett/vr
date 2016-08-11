#pragma once
#include <memory> // shared_ptr
#include <string> // string
#include <SDL.h>
#include "noncopyable.h"
#include "log.h"
#include "signal.h"

namespace core {
  // raii opengl support provider
  struct window : noncopyable {
    inline window(int width, int height) : width(width), height(height) {}
    virtual ~window() {}

    virtual bool poll() = 0; // poll for events, returns true if we should exit.

    int width; // must be maintained
    int height;
  };

  struct sdl_window : window {
    sdl_window(
      std::shared_ptr<spdlog::logger> & log,
      std::string title,
      bool debug = false,
      int x = 700,
      int y = 100,
      int width = 1280,
      int height = 768
    );

    bool poll(); // returns true to indicate we want to quit

    virtual ~sdl_window();

    signal<void()> on_quit, on_app_terminating;
    signal<void(filename_t)> on_drop_file;

    // window events
    signal<void(uint32_t)> on_window_close, on_window_shown, on_window_hidden, on_window_exposed,
      on_window_minimized, on_window_maximized, on_window_restored,
      on_window_enter, on_window_leave, on_window_focus_gained, on_window_focus_lost;
    signal<void(uint32_t, int, int)> on_window_resize, on_window_size_changed, on_window_moved;

    // keyboard events
    signal<void(SDL_KeyboardEvent &)> on_key_down, on_key_up;

    // mouse events
    signal<void(SDL_MouseMotionEvent &)> on_mouse_motion;
    signal<void(SDL_MouseButtonEvent &)> on_mouse_button_down, on_mouse_button_up;
    signal<void(SDL_MouseWheelEvent &)> on_mouse_wheel;

    // joystick events
    signal<void(SDL_JoyButtonEvent &)> on_joy_button_down, on_joy_button_up;
    signal<void(SDL_JoyAxisEvent &)> on_joy_axis_motion;
    signal<void(SDL_JoyBallEvent &)> on_joy_ball_motion;
    signal<void(SDL_JoyHatEvent &)> on_joy_hat_motion;
    signal<void(uint32_t)> on_joy_device_added, on_joy_device_removed;
    signal<void(SDL_ControllerAxisEvent &)> on_controller_axis_motion;
    signal<void(SDL_ControllerButtonEvent &)> on_controller_button_down, on_controller_button_up;
    signal<void(SDL_ControllerDeviceEvent &)> on_controller_device_added, on_controller_device_removed, on_controller_device_remapped;
    // text editing
    signal<void(SDL_TextEditingEvent &)> on_text_editing;
    signal<void(SDL_TextInputEvent &)> on_text_input;

    // audio devices
    signal<void(SDL_AudioDeviceEvent &)> on_audio_device_added, on_audio_device_removed;

    SDL_Window * window;
    SDL_GLContext context;
    std::shared_ptr<spdlog::logger> log;
  };
}