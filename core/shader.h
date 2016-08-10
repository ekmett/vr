#pragma once

#include <string>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <openvr.h>
#include "noncopyable.h"

struct shader : noncopyable {
  shader(const char * name, const char * vertexShader, const char * fragmentShader);
  virtual ~shader();
  GLuint programId; // program id
};


struct rendermodel_shader : shader {
  rendermodel_shader();
};

struct controller_shader : shader {
  controller_shader();
};