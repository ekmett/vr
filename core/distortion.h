#pragma once

#include "shader.h"
#include "window.h"
#include "tracker.h"
#include <glm/glm.hpp>

struct openvr_distortion : shader {
  openvr_distortion(const ::window & window, openvr_tracker & tracker) : openvr_distortion(window, tracker, 0.1f, 30.0f) {}
  openvr_distortion(const ::window & window, openvr_tracker & tracker, float nearZ, float farZ);
  virtual ~openvr_distortion();

  void render();
  void recalculate_ipd();
  // TODO: add callbacks for inter-pupilary distance recalculation

private:
  GLuint vertexArray, vertexBuffer, indexBuffer;
  int windowWidth, windowHeight, indexSize;
  GLuint depthBufferId[2];
  GLuint renderTextureId[2];
  GLuint renderFramebufferId[2];
  GLuint resolutionTextureId[2];
  GLuint resolutionFramebufferId[2];
  glm::mat4 projection[2];
  glm::mat4 pose[2];
  // float nearZ, farZ;
  const window & window;
  openvr_tracker & tracker;
};