#pragma once

#include "config.h"

#include "glew.h"

#ifdef FRAMEWORK_SUPPORTS_SDL2

// I don't want SDL to define M_PI
#define _USE_MATH_DEFINES
#include <math.h>

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