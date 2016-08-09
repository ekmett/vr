#pragma once

#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>

namespace util {
  const char * source(GLenum source);
  const char * message_type(GLenum type);
  void objectLabelf(GLenum id, GLuint name, const char *fmt, ...);
  __declspec(noreturn) void die(const char * function, const char *fmt, ...);
};
