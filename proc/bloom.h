#pragma once

#include "framework/gl.h"
#include "framework/shader.h"

namespace framework {
  struct bloom {
    GLuint fbo[2];
    GLuint texture[2];
    gl::shader pass, downsample, horizontal, vertical, tone;

    bloom(int w, int h)
      : pass(GL_VERTEX_SHADER, "pass", R"(
        #version 450 core       
        #extension GL_ARB_shader_viewport_layer_array : require
        const vec2 positions[4] = vec2[](vec2(-1.0,1.0),vec2(-1.0,-1.0),vec2(1.0,1.0),vec2(1.0,-1.0));
        const vec2 coords[4] = vec2[](vec2(0.0,1.0),vec2(0.0,-1.0),vec2(1.0,1.0),vec2(1.0,0.0));
        noperspective out vec2 coord;
        void main() {
          gl_ViewportIndex = gl_InstanceID;
          vec2 p = positions[gl_VertexId];
          gl_Position = vec4(p,0.0,1.0);
          coord = clamp(p,0.0,1.0);
        }
      )"), downsample(GL_FRAGMENT_SHADER, "downsample", R"(
        // fast factor of 2 downsampling
        #version 450 core       
        in vec2 coord;
        uniform sampler2D render;
        out vec4 outputColor;
        void main() {
          vec4 reds = textureGather(render, coord, 0);
          vec4 greens = textureGather(render, coord, 1);
          vec4 blues = textureGather(render, coord, 2);
          vec3 result;
          for (int i=0;i<4;++i) {
            result += vec3(reds[i], greens[i], blues[i]);
          }
          outputColor = vec4(result / 4.0f, 1.0f);
        }
      )"), horizontal(GL_FRAGMENT_SHADER, "horizontal blur", R"(        
        #version 450 core       
        #extension GL_ARB_shading_language_include : require
        #include "post.h"
        uniform sampler2D input;
        in vec2 coord;
        out vec4 outputColor;
        void main() {
          outputColor = blur(input, coord, vec2(1,0), blur_sigma, false);       
        }
      )"), vertical(GL_FRAGMENT_SHADER, "vertical blur", R"(
        #version 450 core       
        #extension GL_ARB_shading_language_include : require
        #include "post.h"
        uniform sampler2D input;
        in vec2 coord;
        out vec4 outputColor;
        void main() {
          outputColor = blur(input, coord, vec2(0,1), blur_sigma, false);       
        }
      )"), tone(GL_FRAGMENT_SHADER, "tone", R"(
        #version 450 core       
        #extension GL_ARB_shading_language_include : require
        #include "post.h"
        uniform sampler2D render;
        uniform sampler2D bloom;
        in vec2 coord;
        out vec4 outputColor;
        // ALU driven approximation of Duiker's film stock curve
        vec3 filmic(vec3 color) {
          color = max(0, color - 0.004f);
          color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);
          return color;
        }
        void main() {
          vec3 color = texture(render, coord).rgb;
          color += texture(bloom, coord).rgb * bloom_magnitude * exp2(bloom_exposure);
          color *= exp2(exposure) / fp16_scale;                   
          // crush out all the joy in the image with the tonemap
          outputColor = filmic(color);
        }
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
    }
    ~bloom() {
      glDeleteTextures(2, texture);
      glDeleteFramebuffers(2, fbo);
    }

    void tonemap(int render_fbo, int resolve_fbo) {}
      // use separable shader programs for the passes?
  };
}