#pragma once
#include <memory> // shared_ptr
#include <string> // string
#include <SDL.h>
#include "noncopyable.h"
#include "log.h"

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
  SDL_Window * window;
  SDL_GLContext context;
  std::shared_ptr<spdlog::logger> log;
};