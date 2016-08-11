#pragma once

#include <string>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <openvr.h>
#include "noncopyable.h"

namespace core {

  struct shader : noncopyable {
    shader(const char * name, const char * vertexShader, const char * fragmentShader);
    virtual ~shader();
    GLuint programId; // program id
  };

  struct controller_shader : shader {
    controller_shader();
  };
}