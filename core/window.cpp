#include <stdio.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include "window.h"
#include "util.h"

using namespace std;

sdl_window::sdl_window(string title, bool debug, int windowX, int windowY, int windowWidth, int windowHeight) {
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
  window = SDL_CreateWindow(title.c_str(), windowX, windowY, windowWidth, windowHeight, windowFlags);
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
}

sdl_window::~sdl_window() {
  SDL_DestroyWindow(window);
  SDL_Quit();
}