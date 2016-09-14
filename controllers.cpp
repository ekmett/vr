#include "error.h"
#include "gl.h"
#include "glm.h"
#include "openvr.h"
#include "controllers.h"

using namespace framework;
using namespace glm;
using namespace openvr;

namespace framework {
  controllers::controllers() 
    : program("controllers")
    , vao("controller", false, 
      attrib{ 3, GL_FLOAT, GL_FALSE, offsetof(vertex, pos) },
      attrib{ 3, GL_FLOAT, GL_FALSE, offsetof(vertex, color) },
      iattrib{ 1, GL_UNSIGNED_SHORT, offsetof(vertex, controller) }
    ) {
    glUniformBlockBinding(program.programId, 0, 0);    
    std::vector<vertex> verts;
    for (GLushort controller = 0;controller < 2; ++controller) {
      for (int i = 0; i < 3; ++i) {
        vec3 color(0);
        vec4 point(0, 0, 0, 1);
        point[i] += 0.05f;  // X, Y, Z
        color[i] = 1.0;     // R, G, B
        if (controller == 0) color[2] = 1.0;
        verts.push_back(vertex{ vec4(0,0,0,1), color, controller });
        verts.push_back(vertex{ point, color, controller });
      }
      vec3 color(.92f, .92f, .71f);
      verts.push_back(vertex{ vec4(0, 0, -0.02f, 1), color, controller });
      verts.push_back(vertex{ vec4(0, 0, -39.f, 1), color, controller });
    }
    vao.load(verts);
  }

  controllers::~controllers() {
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
    glBindVertexArray(0);
    glLineWidth(1);
    glUseProgram(0);
  }
}