#include <stdio.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include "error.h"
#include "sdl_window.h"
#include "gl.h"

using namespace std;
using namespace spdlog;

namespace framework {
  sdl_gl_window::sdl_gl_window(
    string title, 
    bool debug, 
    int windowX, 
    int windowY, 
    int width, 
    int height
  ) : width(width), 
      height(height) //, 
    //  sdl_video(SDL_INIT_VIDEO) 
    {
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
      die("Window could not be created.\n{}", SDL_GetError());

    // create the gl context
    context = SDL_GL_CreateContext(window);
    if (context == nullptr)
      die("OpenGL context could not be created.\n{}", SDL_GetError());

    // We created or switched an OpenGL context, so initialize glew

    glewExperimental = GL_TRUE;
    auto nGlewError = glewInit();
    if (nGlewError != GLEW_OK)
      die("Error initializing GLEW.\n{}", glewGetErrorString(nGlewError));

    glGetError(); // clear the GLEW error
        
    if (SDL_GL_SetSwapInterval(0) < 0)
      die("Unable to set VSync.\n{}", SDL_GetError());

    if (debug)
      debugger = make_unique<gl::debugger>();

    //SDL_StartTextInput();
    //SDL_ShowCursor(SDL_DISABLE);
  }

  sdl_gl_window::~sdl_gl_window() {
    //SDL_StopTextInput();
    //SDL_ShowCursor(SDL_ENABLE);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
}