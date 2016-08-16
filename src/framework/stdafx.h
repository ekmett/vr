#pragma once

#include <framework/config.h>

// common headers used to build a pch for the framework sub-project
#ifdef FRAMEWORK_USE_STDAFX

// GLEW
#include "framework/glew.h"

// SDL2
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>

#ifdef HAVE_OPENVR
// OpenVR
#include <openvr.h>
#endif

// spdlog and fmtlib
#include "framework/fmt.h"
#include "framework/spdlog.h"

// g-truc's glm
#include "framework/glm.h"

// local stable code
#include "framework/apply.h"
#include "framework/error.h"
#include "framework/utf8.h"
#include "framework/signal.h"
#include "framework/noncopyable.h"

#endif