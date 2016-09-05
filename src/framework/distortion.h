#pragma once

#include "framework/gl.h"
#include "framework/shader.h"

namespace framework {
  // generates a preview of the warped display looks like
  struct distortion {
    distortion(GLushort segmentsH = 43, GLushort segmentsV = 43);
    ~distortion();
    void render_stencil();
    void render(int view_mask);

    void set_resolve_handle(GLuint64 handle) {
      glProgramUniformHandleui64ARB(warp.programId, 0, handle);
    }

    bool debug_wireframe_render_stencil = false;
    bool debug_wireframe_render = false;
  private:
    union {
      GLuint array[2];
      struct { GLuint vao, hidden_vao; };
    };
    int n_indices, n_hidden, n_hidden_left;
    union {
      GLuint buffer[3];
      struct { GLuint vbo, ibo, hidden_vbo; };
    };
    gl::shader mask, warp;
  };
}