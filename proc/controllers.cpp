#include "framework/error.h"
#include "framework/gl.h"
#include "framework/glm.h"
#include "framework/openvr.h"
#include "controllers.h"

using namespace framework;
using namespace glm;
using namespace openvr;

struct vertex {
  vec3 pos;
  vec3 color;
  GLushort controller;
};

controllers::controllers() : program("controllers") {
  glUniformBlockBinding(program.programId, 0, 0);
  glCreateVertexArrays(1, &vao);
  gl::label(GL_VERTEX_ARRAY, vao, "controller vao");
  glCreateBuffers(1, &vbo);
  gl::label(GL_BUFFER, vbo, "controller vbo");
  glEnableVertexArrayAttrib(vao, 0);
  glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(vertex, pos));
  glVertexArrayAttribBinding(vao, 0, 0);
  glEnableVertexArrayAttrib(vao, 1);
  glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(vertex, color));
  glVertexArrayAttribBinding(vao, 1, 0);
  glEnableVertexArrayAttrib(vao, 2);
  glVertexArrayAttribIFormat(vao, 2, 1, GL_UNSIGNED_SHORT, offsetof(vertex, controller));
  glVertexArrayAttribBinding(vao, 2, 0);
  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vertex));
  std::vector<vertex> verts;
  for (GLushort controller = 0;controller < 2; ++ controller) {
    for (int i = 0; i < 3; ++i) {
      vec3 color(0);
      vec4 point(0, 0, 0, 1);
      point[i] += 0.05f;  // X, Y, Z
      color[i] = 1.0; // 1.0;     // R, G, B
      if (controller == 0) color[2] = 1.0; 
      verts.push_back(vertex{ vec4(0,0,0,1), color, controller });
      verts.push_back(vertex{ point, color, controller });
    }
    vec3 color(.92f, .92f, .71f);
    verts.push_back(vertex{ vec4(0, 0, -0.02f, 1), color, controller });
    verts.push_back(vertex{ vec4(0, 0, -39.f, 1), color, controller });
  }
  glNamedBufferData(vbo, sizeof(vertex) * verts.size(), verts.data(), GL_STREAM_DRAW);
}

controllers::~controllers() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void controllers::render(int controller_mask) {
  if (VRSystem()->IsInputFocusCapturedByAnotherProcess()) return;
  glBindVertexArray(vao);
  glLineWidth(6);
  glUseProgram(program.programId);
  switch (controller_mask) {
    case 1: glDrawArraysInstanced(GL_LINES, 0, 8, 2); break;
    case 2: glDrawArraysInstanced(GL_LINES, 8, 8, 2); break;
    case 3: glDrawArraysInstanced(GL_LINES, 0, 16, 2); break;
  }
  glLineWidth(1);
  glUseProgram(0);
}