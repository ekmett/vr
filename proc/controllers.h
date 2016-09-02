#pragma once

#include "framework/glm.h"
#include "framework/shader.h"
#include "uniforms.h"

struct controllers {
  controllers();
  ~controllers();
  void render(int controller_mask);
  framework::gl::shader program;
  GLuint vao;
  GLuint vbo;
};