#pragma once

#include "framework/sdl.h"

#if defined(FRAMEWORK_SUPPORTS_SDL2) && defined(FRAMEWORK_SUPPORTS_OPENGL)

#include <memory> // shared_ptr
#include <string> // string
#include "framework/sdl_system.h"
#include "framework/noncopyable.h"
#include "framework/std.h"
#include "framework/spdlog.h"
#include "framework/signal.h"
#include "framework/gl.h"

namespace framework {
  namespace sdl {
    // raii opengl support provider
    struct window : public system {
      
      window(
        string title,
        gl::version version = { 4, 5, gl::profile::core },
        bool debug = false,
        int x = 300,
        int y = 50,
        int width = 2160,
        int height = 1200
      );

      virtual ~window();

      SDL_Window * sdl_window;
      SDL_GLContext context;
      int width; // must be maintained
      int height;

      unique_ptr<gl::debugger> debugger;

      inline int display_index() const { return SDL_GetWindowDisplayIndex(sdl_window); }

    };
  }
}

#endif