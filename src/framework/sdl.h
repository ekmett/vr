#pragma once

#include "framework/glew.h"

#include <SDL.h>

// we'll just define SDLmain rather than have this take a nice common word away from us!
#undef main

#include <SDL_opengl.h>

#ifdef _WIN32
#pragma comment(lib, "SDL2")
#pragma comment(lib, "SDL2main")
#endif