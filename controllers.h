#pragma once

#include "glm.h"
#include "shader.h"
#include "uniforms.h"
#include "vao.h"

namespace framework {
  struct controllers {
    struct vertex {
      vec3 pos;
      vec3 color;
      GLushort controller;
    };

    controllers();
    ~controllers();
    void render(int controller_mask);
    framework::gl::shader program;
    vertex_array<vertex> vao;
  };
}