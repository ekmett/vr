#pragma once

#include "config.h"

// common headers used to build a pch for the framework sub-project
#ifdef FRAMEWORK_USE_STDAFX

// GLEW
#include "glew.h"

// SDL2
#include "sdl.h"

// OpenVR
#include "openvr.h"

// Oculus SDK
#include "oculus.h"

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

#endif