#pragma once

#ifdef _WIN32
#define USE_GLEW
// GLEW
#include <GL/glew.h>
#endif

// SDL2
#include <SDL.h>
#include <SDL_opengl.h>

// OpenVR
#include <openvr.h>

// spdlog and fmtlib
#include "fmt.h"
#include "spdlog.h"

// g-truc's glm
#include "glm.h"

// local stable code
#include "apply.h"
#include "error.h"
#include "utf8.h"
#include "signal.h"
#include "noncopyable.h"
