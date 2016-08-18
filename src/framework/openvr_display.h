#pragma once

#include "framework/openvr_system.h"
#include "framework/gl.h"
#include "framework/shader.h"

namespace framework {
  namespace openvr {
    // manage eye position and rendering
    struct display {
      display(system & vr, float nearZ = 0.1f, float farZ = 30.0f);
      virtual ~display();

      void submit(bool distort_locally = true);
      float ipd() noexcept; // current inter-pupilary distance in meters

      template <typename f, typename ... T> void stereo(f fun, T ... args);

      void recalculate_pose() noexcept; // used to recalculate pose matrices on ipd changes
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
      system & vr;
      uint32_t renderWidth, renderHeight;
      scoped_connection ipd_connection;
      gl::shader shader;
    };

    template <typename f, typename ... T> void display::stereo(f fun, T ... args) {
      glClearColor(1.f, 0.f, 0.f, 1.f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);

      //glClearColor(0.15f, 0.15f, 0.18f, 1.0f); // nice background color, but not black

      for (int i = 0;i < 2;++i) {
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
      submit();
    }
  }
} // core