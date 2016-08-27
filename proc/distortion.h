#pragma once

#include "framework/gl.h"
#include "framework/shader.h"

struct distortion {
  distortion(GLushort segmentsH = 43, GLushort segmentsV = 43);
  ~distortion();
  void render(GLuint resolutionTexture);
private:
  GLuint vao;
  framework::gl::shader program;
  int n_indices;
  union {
    GLuint buffer[2];
    struct { GLuint vbo, ibo; };
  };
};