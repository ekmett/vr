#pragma once

#if defined(_APPLE)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#define USE_GLEW
#include <GL/glew.h>
#endif

#ifdef _WIN32
#pragma comment(lib, "glew32")
#pragma comment(lib, "opengl32")
#include <GL/wglew.h>
#endif
