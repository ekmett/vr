#pragma once
#include <memory> // shared_ptr
#include <string> // string
#include <SDL.h>
#include "noncopyable.h"
#include "log.h"
#include "signal.h"

// raii opengl support provider
struct window : noncopyable {
  inline window(int width, int height) : width(width), height(height) {}
  virtual ~window() {}
  virtual bool poll() = 0;

  int width; // must be maintained
  int height;
};

struct sdl_window : window {
  sdl_window(std::shared_ptr<spdlog::logger> & log, std::string title, bool debug = false, int x = 700, int y = 100, int width = 1280, int height = 768);
  bool poll(); // returns true to indicate we want to quit
  virtual ~sdl_window();

  signal<void()> on_quit;
  signal<void(filename_t)> on_drop_file;
  signal<void(uint32_t)> on_window_close, on_window_shown, on_window_hidden, on_window_exposed, on_window_minimized, on_window_maximized, on_window_restored, on_window_leave, on_window_focus_gained, on_window_focus_lost;
  signal<void(uint32_t, int, int)> on_window_resize, on_window_size_changed, on_window_moved;
  signal<void(SDL_KeyboardEvent &)> on_key_down, on_key_up;
  signal<void(SDL_MouseMotionEvent &)> on_mouse_motion;
  signal<void(SDL_MouseButtonEvent &)> on_mouse_button_down, on_mouse_button_up;

  SDL_Window * window;
  SDL_GLContext context;
  std::shared_ptr<spdlog::logger> log;
};