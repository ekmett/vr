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
};

controllers::controllers()
  : program("controller", R"(
      #version 450
      #extension GL_ARB_shader_viewport_layer_array : enable
      uniform mat4 matrix[2];
      layout(location = 0) in vec4 position;
      layout(location = 1) in vec3 v3ColorIn;
      out vec4 v4Color;
      void main() {
    	  v4Color.xyz = v3ColorIn; v4Color.a = 1.0;
    	  gl_Position = matrix[gl_InstanceID] * position;
        gl_ViewportIndex = gl_InstanceID;
      }
    )", R"(
      #version 450
      in vec4 v4Color;
      out vec4 outputColor;
      void main() {
        outputColor = v4Color;
      }
    )") {

  matrixUniformLocation = glGetUniformLocation(program.programId, "matrix");
  if (matrixUniformLocation == -1) die("unable to find matrix uniform");

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

  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vertex));
}

controllers::~controllers() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void controllers::render(vr::TrackedDevicePose_t pose[vr::k_unMaxTrackedDeviceCount], mat4 eyeViewProjections[2]) {
  auto hmd = VRSystem();

  // don't draw controllers if somebody else has input focus
  if (VRSystem()->IsInputFocusCapturedByAnotherProcess()) return;

  std::vector<vertex> verts;

  int m_iTrackedControllerCount = 0;

  for (auto i = vr::k_unTrackedDeviceIndex_Hmd + 1; i < vr::k_unMaxTrackedDeviceCount; ++i) {
    if (!hmd->IsTrackedDeviceConnected(i)) continue;
    if (hmd->GetTrackedDeviceClass(i) != vr::TrackedDeviceClass_Controller) continue;
    m_iTrackedControllerCount += 1;
    if (!pose[i].bPoseIsValid) continue;

    mat4 mat = hmd_mat3x4(pose[i].mDeviceToAbsoluteTracking);
    vec4 center = mat * vec4(0, 0, 0, 1);

    for (int i = 0; i < 3; ++i) {
      vec3 color(0, 0, 0);
      vec4 point(0, 0, 0, 1);
      point[i] += 0.05f;  // offset in X, Y, Z
      color[i] = 1.0;     // R, G, B
      point = mat * point;
      verts.push_back(vertex{ center, color });
      verts.push_back(vertex{ point, color });
    }

    vec3 color(.92f, .92f, .71f);
    verts.push_back(vertex{ mat * vec4(0, 0, -0.02f, 1), color });
    verts.push_back(vertex{ mat * vec4(0, 0, -39.f, 1), color });
  }

  if (verts.size() > 0) {
    glNamedBufferData(vbo, sizeof(vertex) * verts.size(), verts.data(), GL_STREAM_DRAW); // switch to BufferSubData?

    glUseProgram(program.programId);
    glUniformMatrix4fv(matrixUniformLocation, 2, GL_FALSE, &eyeViewProjections[0][0][0]);
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_LINES, 0, verts.size(), 2);
    glBindVertexArray(0);
    glUseProgram(0);
  }
}



