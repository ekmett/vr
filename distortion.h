#pragma once

#include "gl.h"
#include "shader.h"
#include "vao.h"

namespace framework {
  // generates a preview of the warped display looks like
  struct distortion {
    struct vertex {
      vec2 p, r, g, b;
      GLushort eye;
    };

    distortion(GLushort segmentsH = 43, GLushort segmentsV = 43);
    ~distortion();
    void render_stencil();
    void render(int view_mask, GLuint64 handle, float render_buffer_usage);
    bool debug_wireframe_render_stencil = false;
    bool debug_wireframe_render = false;
  private:
    vertex_array<vertex> vao;
    vertex_array<vec2> hidden_vao;
    int n_indices, n_hidden, n_hidden_left;
    gl::shader mask, warp;
  };
}