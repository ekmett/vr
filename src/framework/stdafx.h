#pragma once

#include "framework/config.h"

// common headers used to build a pch for the framework sub-project
#ifdef FRAMEWORK_USE_STDAFX

// GLEW
#include "framework/glew.h"

// SDL2
#include "framework/sdl.h"

// OpenVR
#include "framework/openvr.h"

// Oculus SDK
#include "framework/oculus.h"

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