#pragma once

#include "shader.h"
#include "window.h"
#include "tracker.h"
#include <glm/glm.hpp>

// manage eye position and rendering
struct openvr_distortion : shader {
  openvr_distortion(const ::window & window, openvr_tracker & tracker) : openvr_distortion(window, tracker, 0.1f, 30.0f) {}
  openvr_distortion(const ::window & window, openvr_tracker & tracker, float nearZ, float farZ);
  virtual ~openvr_distortion();

  void render();
  void recalculate_ipd();
  float ipd(); // inter-pupilary distance in meters
  
  template <typename f, typename ... T> void stereo(f fun, T ... args) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);



    glClearColor(0.15f, 0.15f, 0.18f, 1.0f); // nice background color, but not black

    for (int i=0;i<2;++i) {
      glEnable(GL_MULTISAMPLE);
      glBindFramebuffer(GL_FRAMEBUFFER, renderFramebufferId[i]);
      glViewport(0, 0, renderWidth, renderHeight);
      fun(eye(i), args...);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDisable(GL_MULTISAMPLE);

      glBindFramebuffer(GL_READ_FRAMEBUFFER, renderFramebufferId[i]);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolutionFramebufferId[i]);
      glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
    
    render();
  }

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
  const window & window;
  openvr_tracker & tracker;
  uint32_t renderWidth, renderHeight;
};