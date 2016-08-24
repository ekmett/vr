#pragma once

#include "framework/config.h"

#include "framework/glew.h"

#ifdef FRAMEWORK_SUPPORTS_SDL2

#define SDL_MAIN_HANDLED
#include <SDL.h>

#ifdef _WIN32
#pragma comment(lib, "SDL2")
#pragma comment(lib, "SDL2main")
#endif

#ifdef FRAMEWORK_SUPPORTS_OPENGL
#include <SDL_opengl.h>
#endif

#endif