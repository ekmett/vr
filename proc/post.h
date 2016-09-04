#pragma once

#include "framework/gl.h"
#include "framework/shader.h"

namespace framework {
  struct post {
    GLuint fbo[2];
    GLuint texture[2];
    gl::shader pass, downsample, horizontal, vertical, tone;

    post(int w, int h)
      : pass(GL_VERTEX_SHADER, "post_pass")
      , downsample(GL_FRAGMENT_SHADER, "post_downsample")
      , horizontal(GL_FRAGMENT_SHADER, "post_blur_h")
      , vertical(GL_FRAGMENT_SHADER, "post_blur_v")
      , tone(GL_FRAGMENT_SHADER, "post_tonemap", R"(
      )") {
      glCreateFramebuffers(2, fbo);
      glCreateTextures(GL_TEXTURE_2D, 2, texture);
      for (int i = 0;i < 2;++i) {
        gl::label(GL_FRAMEBUFFER, fbo[i], "bloom fbo {}", i);
        gl::label(GL_TEXTURE, texture[i], "bloom texture {}", i);
        glTextureParameteri(texture[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture[i], GL_TEXTURE_MAX_LEVEL, 0);
        glTextureImage2DEXT(texture[i], GL_TEXTURE_2D, 0, GL_RGBA16F, w, h / 2, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glNamedFramebufferTexture(fbo[i], GL_COLOR_ATTACHMENT0, texture[i], 0);
      }
      glProgramUniform1f(
    }
    ~post() {
      glDeleteTextures(2, texture);
      glDeleteFramebuffers(2, fbo);
    }

    void tonemap(int render_fbo, int resolve_fbo) {
    
    }
      // use separable shader programs for the passes?
  };
}