#pragma once

#include <memory> // shared_ptr
#include <string> // string
#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include "sdl_system.h"
#include "noncopyable.h"
#include "std.h"
#include "spdlog.h"
#include "signal.h"
#include "gl.h"

namespace framework {
  namespace sdl {
    // raii opengl support provider
    struct gl_window : noncopyable {
      
      gl_window(
        string title,
        gl::version version = { 4, 5, gl::profile::core },
        bool debug = false,
        int x = 700,
        int y = 100,
        int width = 1280,
        int height = 768
      );

      virtual ~gl_window();

     //  inline bool poll() { return video_system.poll(); }

      SDL_Window * window;
      SDL_GLContext context;
      int width; // must be maintained
      int height;

      std::unique_ptr<gl::debugger> debugger;
    };
  }
}