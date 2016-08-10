#pragma once
#include <string>
#include <SDL.h>
#include "noncopyable.h"

// raii opengl support provider
struct window : noncopyable {
  window() {}
  virtual ~window() {}

  int width; // must be maintained
  int height;
};

struct sdl_window : window {
  sdl_window(std::string title, bool debug = false, int x = 700, int y = 100, int width = 1280, int height = 768);
  virtual ~sdl_window();
  SDL_Window * window;
  SDL_GLContext context;
};