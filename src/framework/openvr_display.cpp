#include "framework/stdafx.h"
#include "framework/openvr_display.h"
#include "framework/gl.h"
#include "framework/std.h"

namespace framework {
  namespace openvr {

    using namespace vr;
    using namespace glm;
    using namespace std;

    static const int msaa_quality = 4;

    display::display(system & vr, float nearZ, float farZ)
      : vr(vr)
      , ipd_connection(vr.on_ipd_changed.connect([this] { recalculate_pose(); }))
      , shader("distortion",
        R"(#version 410 core
     layout(location = 0) in vec4 position;
     layout(location = 1) in vec2 v2UVredIn;
     layout(location = 2) in vec2 v2UVGreenIn;
     layout(location = 3) in vec2 v2UVblueIn;
     noperspective  out vec2 v2UVred;
     noperspective  out vec2 v2UVgreen;
     noperspective  out vec2 v2UVblue;
     void main() {
     	 v2UVred = v2UVredIn;
     	 v2UVgreen = v2UVGreenIn;
     	 v2UVblue = v2UVblueIn;
     	 gl_Position = position;
     })",
        R"(#version 410 core
     uniform sampler2D mytexture;
     noperspective in vec2 v2UVred;
     noperspective in vec2 v2UVgreen;
     noperspective in vec2 v2UVblue;
     out vec4 outputColor;

     void main() {
       float fBoundsCheck = ( (dot( vec2( lessThan( v2UVgreen.xy, vec2(0.05, 0.05)) ), vec2(1.0, 1.0))+dot( vec2( greaterThan( v2UVgreen.xy, vec2( 0.95, 0.95)) ), vec2(1.0, 1.0))) );
       if( fBoundsCheck > 1.0 ) {
       	 outputColor = vec4( 0, 0, 0, 1.0 );
       } else {
         float red = texture(mytexture, v2UVred).x;
         float green = texture(mytexture, v2UVgreen).y;
         float blue = texture(mytexture, v2UVblue).z;
         outputColor = vec4( red, green, blue, 1.0  ); 
       }
     })") {

      // how big does the hmd want the render target to be, anyways?

      vr.handle->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

      // allocate eyes  
      for (int i = 0;i < 2;++i) {
        auto e = eye(i);
        projection[i] = hmd_mat4(vr.handle->GetProjectionMatrix(e, nearZ, farZ, API_OpenGL));
        pose[i] = hmd_mat3x4(vr.handle->GetEyeToHeadTransform(e));
      }

      glGenFramebuffers(2, renderFramebufferId);
      glGenRenderbuffers(2, depthBufferId);
      glGenTextures(2, renderTextureId);
      glGenFramebuffers(2, resolutionFramebufferId);
      glGenTextures(2, resolutionTextureId);

      // build eyes so we can see
      for (int i = 0;i < 2;++i) {
        const char * l = show_eye(i);
        glBindFramebuffer(GL_FRAMEBUFFER, renderFramebufferId[i]);
        gl::label(GL_FRAMEBUFFER, renderFramebufferId[i], "{} eye render fbo", l);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBufferId[i]);
        gl::label(GL_RENDERBUFFER, depthBufferId[i], "{} eye depth", l);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_quality, GL_DEPTH_COMPONENT, renderWidth, renderHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferId[i]);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderTextureId[i]);
        gl::label(GL_TEXTURE, renderTextureId[i], "{} eye render", l);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaa_quality, GL_RGBA8, renderWidth, renderHeight, true);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, renderTextureId[i], 0);
        glBindFramebuffer(GL_FRAMEBUFFER, resolutionFramebufferId[i]);
        gl::label(GL_FRAMEBUFFER, resolutionFramebufferId[i], "{} eye resolution fbo", l);
        glBindTexture(GL_TEXTURE_2D, resolutionTextureId[i]);
        gl::label(GL_TEXTURE, resolutionTextureId[i], "{} eye resolution", l);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, renderWidth, renderHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolutionTextureId[i], 0);
      }

      // check FBO status
      GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if (status != GL_FRAMEBUFFER_COMPLETE)
        die("Unable to construct your eyes");

      // let it go
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      GLushort segmentsH = 43, segmentsV = 43;

      float w = (float)(1.0 / float(segmentsH - 1)),
        h = (float)(1.0 / float(segmentsV - 1));

      float u, v = 0;

      struct vertex {
        vec2 p, r, g, b;
      };

      vector<vertex> verts(0);
      vertex vert;

      //left eye distortion verts
      for (int i = 0;i < 2;++i) {
        float Xoffset = float(i) - 1; // -1 for left, 0 for right.
        for (int y = 0; y < segmentsV; ++y) {
          for (int x = 0; x < segmentsH; ++x) {
            u = x*w; v = 1 - y*h;
            vert.p = vec2(Xoffset + u, -1 + 2 * y*h);

            vr::DistortionCoordinates_t dc0 = vr.handle->ComputeDistortion(vr::Eye_Left, u, v);

            vert.r = vec2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
            vert.g = vec2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
            vert.b = vec2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

            verts.push_back(vert);
          }
        }
      }

      vector<GLushort> indices;

      for (int i = 0; i < 2; ++i) {
        GLushort offset = i*segmentsH*segmentsV;
        for (GLushort y = 0; y < segmentsV - 1; y++) {
          for (GLushort x = 0; x < segmentsH - 1; x++) {
            GLushort a = segmentsH*y + x + offset;
            GLushort b = segmentsH*y + x + 1 + offset;
            GLushort c = (y + 1)*segmentsH + x + 1 + offset;
            GLushort d = (y + 1)*segmentsH + x + offset;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);

            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(d);
          }
        }
      }

      indexSize = int(indices.size());

      glGenVertexArrays(1, &vertexArray);
      glBindVertexArray(vertexArray);
      gl::label(GL_VERTEX_ARRAY, vertexArray, "display distortion vao");

      glGenBuffers(1, &vertexBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
      gl::label(GL_BUFFER, vertexBuffer, "display distortion vbo");
      glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vertex), &verts[0], GL_STATIC_DRAW);

      glGenBuffers(1, &indexBuffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
      gl::label(GL_BUFFER, indexBuffer, "display distortion ibo");
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), &indices[0], GL_STATIC_DRAW);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, p));

      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, r));

      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, g));

      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, b));

      glBindVertexArray(0);

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
      glDisableVertexAttribArray(2);
      glDisableVertexAttribArray(3);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    display::~display() {
      glDeleteBuffers(1, &vertexBuffer);
      glDeleteBuffers(1, &indexBuffer);
      glDeleteVertexArrays(1, &vertexArray);
      glDeleteRenderbuffers(2, depthBufferId);
      glDeleteTextures(2, renderTextureId);
      glDeleteFramebuffers(2, renderFramebufferId);
      glDeleteTextures(2, resolutionTextureId);
      glDeleteFramebuffers(2, resolutionFramebufferId);
    }

    void display::submit(bool distort_locally) {
    }

    void display::recalculate_pose() noexcept {
      for (int i = 0;i < 2;++i)
        pose[i] = hmd_mat3x4(vr.handle->GetEyeToHeadTransform(eye(i)));
    }

    float display::ipd() noexcept {
      return length(pose[0][3] - pose[1][3]); // magnitude of the difference between the translation components of the eye-to-head matrices.
    }
  }
}