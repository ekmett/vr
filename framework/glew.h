#pragma once

#if defined(_APPLE)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#define USE_GLEW
#include <GL/glew.h>
#endif

#ifdef _WIN32
#include <GL/wglew.h>
#endif
