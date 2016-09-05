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

      glUseProgramStages(downsample_pipeline, GL_VERTEX_SHADER_BIT, pass.programId);
      glUseProgramStages(downsample_pipeline, GL_FRAGMENT_SHADER_BIT, downsample.programId);
      glProgramUniformHandleui64ARB(downsample.programId, 0, presolve.texture_handle);

      glUseProgramStages(horizontal_pipeline, GL_VERTEX_SHADER_BIT, pass.programId);
      glUseProgramStages(horizontal_pipeline, GL_FRAGMENT_SHADER_BIT, horizontal.programId);
      glProgramUniformHandleui64ARB(horizontal.programId, 0, fbo[0].texture_handle);

      glUseProgramStages(vertical_pipeline, GL_VERTEX_SHADER_BIT, pass.programId);
      glUseProgramStages(vertical_pipeline, GL_FRAGMENT_SHADER_BIT, vertical.programId);
      glProgramUniformHandleui64ARB(vertical.programId, 0, fbo[1].texture_handle);

      glUseProgramStages(tone_pipeline, GL_VERTEX_SHADER_BIT, pass.programId);
      glUseProgramStages(tone_pipeline, GL_FRAGMENT_SHADER_BIT, tone.programId);
      glProgramUniformHandleui64ARB(tone.programId, 0, presolve.texture_handle);
      glProgramUniformHandleui64ARB(tone.programId, 1, fbo[0].texture_handle);
    }

    ~post() {
      glDeleteProgramPipelines(countof(pipelines), pipelines);
    }

    void process() {
      log("post")->info("process() start");
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_STENCIL_TEST);
      glDisable(GL_MULTISAMPLE);
      glDisable(GL_BLEND);
      glEnable(GL_SCISSOR_TEST);
      glViewport(0, 0, quality.viewport_w, quality.viewport_h);
      glScissor(0, 0, quality.viewport_w, quality.viewport_h);            
      glUseProgram(0); // programs trump pipelines

      // downsample
      log("post")->info("downsampling");  
      fbo[0].bind();
      glBindProgramPipeline(downsample_pipeline);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);
      
      // blur
      for (int i = 0;i < 0;++i) {
        // horizontal
        glBindProgramPipeline(horizontal_pipeline);
        fbo[1].bind();
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);

        // vertical
        glBindProgramPipeline(vertical_pipeline);
        fbo[0].bind();
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);
      }

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
    GLsizei w, h;

    quality & quality;
  };
}