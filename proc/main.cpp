#include <string>
#include <stdexcept>
#include <functional>
#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <gl/glu.h>
#include <windows.h>
#include "../framework/openvr.h"
#include "../framework/sdl.h"

using namespace framework;

int main(int argc, char ** argv) {
  openvr vr;
  sdl sdl;  
  return 0;

  /*
  // once created OpenGL works
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    printf("Unable to initialize SDL.\n%s\n", SDL_GetError());
    exit(1);
  }

  finally<> shutdown_sdl(SDL_Quit);

  Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

  auto window = SDL_CreateWindow("proc", 0, 0, 1280, 768, windowFlags);
  if (window == nullptr) die("Window could not be created.\n%s\n", SDL_GetError());

  auto context = SDL_GL_CreateContext(window);
  if (context == nullptr) die("OpenGL context could not be created.\n%s\n", SDL_GetError());

  // initialize glew
  glewExperimental = GL_TRUE;
  auto nGlewError = glewInit();
  if (nGlewError != GLEW_OK) die("Error initializing GLEW.\n%s\n", (const char *)glewGetErrorString(nGlewError));
  glGetError(); // to clear the error caused deep in GLEW

  if (SDL_GL_SetSwapInterval(0) < 0) die("Unable to set VSync.\n%s\n", SDL_GetError());

  SDL_StartTextInput();
  finally<> stop_text_input(SDL_StopTextInput);

  SDL_ShowCursor(SDL_DISABLE);
  finally<int> show_cursor(SDL_ShowCursor, SDL_ENABLE);




  return 0;

  */
}