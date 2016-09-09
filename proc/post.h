#pragma once

#include "gl.h"
#include "quality.h"
#include "shader.h"
#include "timer.h"
#include <stb_image.h>

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
    GLuint flare;
    GLuint64 flare_handle;

    post(quality & quality)
      : quality(quality)
      , pass(GL_VERTEX_SHADER, "post_pass")
      , downsample(GL_FRAGMENT_SHADER, "post_downsample")
      , horizontal(GL_FRAGMENT_SHADER, "post_blur_h")
      , vertical(GL_FRAGMENT_SHADER, "post_blur_v")
      , tone(GL_FRAGMENT_SHADER, "post_tonemap")
      , w((quality.resolve_buffer_w + 1)/ 2)
      , h((quality.resolve_buffer_h + 1)/ 2)
      , presolve(quality.resolve_target.format, "presolve", GL_RGBA16F)
      , timer("post") {

      glCreateVertexArrays(1, &vao);
            
      for (int i = 0;i < 2;++i) {
        fbo[i].format = { w, h };
        string name = fmt::format("post fbo {}", i);
        fbo[i].initialize(name, GL_RGBA16F);
        //fbo[i].initialize(name, GL_R11F_G11F_B10F);
      }
      
      glCreateProgramPipelines(countof(pipelines), pipelines);

      glUniformBlockBinding(pass, 0, 0);

      glUseProgramStages(downsample_pipeline, GL_VERTEX_SHADER_BIT, pass);
      glUseProgramStages(downsample_pipeline, GL_FRAGMENT_SHADER_BIT, downsample);
      glProgramUniformHandleui64ARB(downsample, 0, presolve.texture_handle);
      glUniformBlockBinding(downsample, 0, 0);

      glUseProgramStages(horizontal_pipeline, GL_VERTEX_SHADER_BIT, pass);
      glUseProgramStages(horizontal_pipeline, GL_FRAGMENT_SHADER_BIT, horizontal);
      glProgramUniformHandleui64ARB(horizontal, 0, fbo[0].texture_handle);
      glUniformBlockBinding(horizontal, 0, 0);

      glUseProgramStages(vertical_pipeline, GL_VERTEX_SHADER_BIT, pass);
      glUseProgramStages(vertical_pipeline, GL_FRAGMENT_SHADER_BIT, vertical);
      glProgramUniformHandleui64ARB(vertical, 0, fbo[1].texture_handle);
      glUniformBlockBinding(vertical, 0, 0);

      glUseProgramStages(tone_pipeline, GL_VERTEX_SHADER_BIT, pass);
      glUseProgramStages(tone_pipeline, GL_FRAGMENT_SHADER_BIT, tone);
      glProgramUniformHandleui64ARB(tone, 0, presolve.texture_handle);
      glProgramUniformHandleui64ARB(tone, 1, fbo[0].texture_handle);
      glUniformBlockBinding(tone, 0, 0);

      {
        int w, h, comp;
        auto image = stbi_load("images/lensflare_1d.png", &w, &h, &comp, 4);
        if (!image) die("needs more lensflare");
        assert(h == 1);
        log("setup")->info("loaded 1d flare: w = {}, components = {}", w, comp);
        glCreateTextures(GL_TEXTURE_1D, 1, &flare);
        glTextureStorage1D(flare, 1, GL_RGB8, w);
        glTextureSubImage1D(flare, 0, 0, w, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glTextureParameteri(flare, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(flare, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(flare, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glGenerateTextureMipmap(flare);
        flare_handle = glGetTextureHandleARB(flare);
        glMakeTextureHandleResidentARB(flare_handle);
        glProgramUniformHandleui64ARB(tone, 2, flare_handle);
        stbi_image_free(image);
      }
    }

    ~post() {
      glMakeTextureHandleNonResidentARB(flare_handle);
      glDeleteTextures(1, &flare);
      glDeleteVertexArrays(1, &vao);
      glDeleteProgramPipelines(countof(pipelines), pipelines);
    }

    void process() {
      static elapsed_timer post_timer("post");
      timer_block timed(post_timer);
      int vw = quality.viewport_w, vh = quality.viewport_h;
      pw = vw / 2;
      ph = vh / 2;
      log("post")->info("process() start");
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_STENCIL_TEST);
      glDisable(GL_SCISSOR_TEST);
      glDisable(GL_MULTISAMPLE);
      glDisable(GL_BLEND);

      glBindVertexArray(vao);
      glUseProgram(0); // programs trump pipelines

      glClearColor(0, 0, 0, 0); // we don't clear the presolve as we only sample it at points, but we blur in here
      fbo[1].bind();
      glClear(GL_COLOR_BUFFER_BIT);
      fbo[0].bind();
      glClear(GL_COLOR_BUFFER_BIT);

      glEnable(GL_SCISSOR_TEST);

      {
        static elapsed_timer downsample_timer("downsample");
        timer_block downsample_timing(downsample_timer);

        // downsample
        log("post")->info("downsampling");
        glViewport(0, 0, w, h);
        glScissor(0, 0, pw, ph);

        for (int i = 0;i < 2;++i)
          glBlitNamedFramebuffer(presolve.fbo_view[i], fbo[0].fbo_view[i], 0, 0, vw, vh, 0, 0, pw, ph, GL_COLOR_BUFFER_BIT, GL_LINEAR);
      }

      {
        static elapsed_timer timer[2] { { "blur pass 0" }, { "blur pass 1" } };
        for (int i = 0;i < 2;++i) {
          timer_block downsample_timing(timer[i]);
          // blur
          // horizontal
          glBindProgramPipeline(horizontal_pipeline);
          fbo[1].bind();
          glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 2);

          // vertical
          glBindProgramPipeline(vertical_pipeline);
          fbo[0].bind();
          glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 2);
        }
      }

      {
        static elapsed_timer timer("tonemap");
        timer_block timing(timer);

        glViewport(0, 0, quality.resolve_buffer_w, quality.resolve_buffer_h);
        glScissor(0, 0, vw, vh);

        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        log("post")->info("tonemap");
        glBindProgramPipeline(tone_pipeline);
        quality.resolve_target.bind();
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 2);

        glBindVertexArray(0);
        glBindProgramPipeline(0);
      }
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
    GLuint vao;

    quality & quality;
    elapsed_timer timer;
  };
}