#pragma once

#include "framework/glm.h"
#include "framework/shader.h"

struct controllers {
  controllers();
  ~controllers();

  void render(vr::TrackedDevicePose_t pose[vr::k_unMaxTrackedDeviceCount], glm::mat4 eyeViewProjections[2]);

  framework::gl::shader program;
  GLuint matrixUniformLocation;
  GLuint vao;
  GLuint vbo;
};