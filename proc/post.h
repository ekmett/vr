#pragma once

#include "framework/gl.h"
#include "framework/quality.h"
#include "framework/shader.h"

namespace framework {
  struct post {

    // render the hidden area stencil into a shared stencil buffer for post?

    // blit:       render -> presolve done during quality.resolve()

    // downsample: presolve -> 0
    // horizontal: 0 -> 1
    // vertical:   1 -> 0
    // horizontal: 0 -> 1
    // vertical:   1 -> 0
    // tonemap ::  {0, presolve} -> resolve

    stereo_fbo presolve;
    stereo_fbo fbo[2];

    post(quality & quality)
      : quality(quality)
      , pass(GL_VERTEX_SHADER, "post_pass")
      , downsample(GL_FRAGMENT_SHADER, "post_downsample")
      , horizontal(GL_FRAGMENT_SHADER, "post_blur_h")
      , vertical(GL_FRAGMENT_SHADER, "post_blur_v")
      , tone(GL_FRAGMENT_SHADER, "post_tonemap")
      , w((quality.resolve_buffer_w + 1)/ 2)
      , h((quality.resolve_buffer_h + 1)/ 2)
      , presolve(quality.resolve_target.format, "presolve", GL_RGBA16F) {
      
      for (int i = 0;i < 2;++i) {
        fbo[i].format = { w, h };
        fbo[i].initialize("post fbo 0", GL_RGBA16F);
      }
      
      glCreateProgramPipelines(countof(pipelines), pipelines);

      glUniformBlockBinding(pass.programId, 0, 0);

      glUseProgramStages(downsample_pipeline, GL_VERTEX_SHADER_BIT, pass.programId);
      glUseProgramStages(downsample_pipeline, GL_FRAGMENT_SHADER_BIT, downsample.programId);
      glProgramUniformHandleui64ARB(downsample.programId, 0, presolve.texture_handle);
      glUniformBlockBinding(downsample.programId, 0, 0);

      glUseProgramStages(horizontal_pipeline, GL_VERTEX_SHADER_BIT, pass.programId);
      glUseProgramStages(horizontal_pipeline, GL_FRAGMENT_SHADER_BIT, horizontal.programId);
      glProgramUniformHandleui64ARB(horizontal.programId, 0, fbo[0].texture_handle);
      glUniformBlockBinding(horizontal.programId, 0, 0);

      glUseProgramStages(vertical_pipeline, GL_VERTEX_SHADER_BIT, pass.programId);
      glUseProgramStages(vertical_pipeline, GL_FRAGMENT_SHADER_BIT, vertical.programId);
      glProgramUniformHandleui64ARB(vertical.programId, 0, fbo[1].texture_handle);
      glUniformBlockBinding(vertical.programId, 0, 0);

      glUseProgramStages(tone_pipeline, GL_VERTEX_SHADER_BIT, pass.programId);
      glUseProgramStages(tone_pipeline, GL_FRAGMENT_SHADER_BIT, tone.programId);
      glProgramUniformHandleui64ARB(tone.programId, 0, presolve.texture_handle);
      glProgramUniformHandleui64ARB(tone.programId, 1, fbo[0].texture_handle);
      glUniformBlockBinding(tone.programId, 0, 0);
    }

    ~post() {
      glDeleteProgramPipelines(countof(pipelines), pipelines);
    }

    void process() {
      int vw = quality.viewport_w, vh = quality.viewport_h;
      pw = vw / 2;
      ph = vh / 2;
      log("post")->info("process() start");
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_STENCIL_TEST);
      glDisable(GL_SCISSOR_TEST);
      glDisable(GL_MULTISAMPLE);
      glDisable(GL_BLEND);
      glUseProgram(0); // programs trump pipelines

      glClearColor(0, 0, 0, 0.0); // we don't clear the presolve as we only sample it at points, but we blur in here
      fbo[1].bind();
      glClear(GL_COLOR_BUFFER_BIT);
      fbo[0].bind();
      glClear(GL_COLOR_BUFFER_BIT);

      glEnable(GL_SCISSOR_TEST);

      // downsample
      log("post")->info("downsampling");  
      glViewport(0, 0, w, h);
      glScissor(0, 0, pw, ph);

      for (int i = 0;i < 2;++i)
        glBlitNamedFramebuffer(presolve.fbo_view[i], fbo[0].fbo_view[i], 0, 0, vw, vh, 0, 0, pw, ph, GL_COLOR_BUFFER_BIT, GL_LINEAR);

      // blur
      for (int i = 0;i < 2;++i) {
        // horizontal
        glBindProgramPipeline(horizontal_pipeline);
        fbo[1].bind();
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);

        // vertical
        glBindProgramPipeline(vertical_pipeline);
        fbo[0].bind();
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);
      }

      glViewport(0, 0, quality.resolve_buffer_w, quality.resolve_buffer_h);
      glScissor(0, 0, vw, vh);

      glEnable(GL_BLEND);

      log("post")->info("tonemap");
      glBindProgramPipeline(tone_pipeline);
      quality.resolve_target.bind();
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);

      glBindProgramPipeline(0);
      log("post")->info("process() end");
    }
    union {
      GLuint pipelines[4];
      struct {
        GLuint downsample_pipeline, horizontal_pipeline, vertical_pipeline, tone_pipeline;
      };
    };

    gl::shader pass, downsample, horizontal, vertical, tone;
    GLsizei w, h; // buffer width
    GLsizei pw, ph; // viewport

    quality & quality;
  };
}