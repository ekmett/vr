#pragma once

#include "framework/gl.h"
#include "framework/shader.h"

// generates a preview of the warped display looks like
struct distortion {
  distortion(GLushort segmentsH = 83, GLushort segmentsV = 83);
  ~distortion();
  void render_stencil();
  void render(GLuint resolveTexture);
private:
  union {
    GLuint array[2];
    struct { GLuint vao, hidden_vao; };
  };
  int n_indices, n_hidden;
  union {
    GLuint buffer[3];
    struct { GLuint vbo, ibo, hidden_vbo; };
  };
  framework::gl::shader mask, warp;
};