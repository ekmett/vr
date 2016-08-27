#include "framework/glm.h"
#include "framework/std.h"
#include "framework/sdl_window.h"
#include "framework/openvr.h"
#include "distortion.h"

using namespace framework;
using namespace glm;
using namespace vr;

distortion::distortion(GLushort segmentsH , GLushort segmentsV) 
  : program("distortion",
    R"(
      #version 410 core
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
      }
    )", R"(
      #version 410 core
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
      }
    )") {

  float w = (float)(1.0 / float(segmentsH - 1)),
        h = (float)(1.0 / float(segmentsV - 1));

  struct vertex {
    vec2 p, r, g, b;
  };

  vector<vertex> verts(0);

  for (int i = 0;i < 2;++i) {
    float o = float(i) - 1; // -1 for left eye, 0 for right
    for (int y = 0; y < segmentsV; ++y) {
      for (int x = 0; x < segmentsH; ++x) {
        float u = x*w, v = 1 - y*h;
        DistortionCoordinates_t dc0 = VRSystem()->ComputeDistortion(vr::EVREye(i), u, v);
        verts.push_back(vertex {
          vec2(o + u, 2*y*h-1),
          vec2(o + 0.5 * dc0.rfRed[0],   1 - dc0.rfRed[1]),
          vec2(o + 0.5 * dc0.rfGreen[0], 1 - dc0.rfGreen[1]),
          vec2(o + 0.5 * dc0.rfBlue[0],  1 - dc0.rfBlue[1])
        });
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

  n_indices = int(indices.size());

  glCreateVertexArrays(1, &vao);
  gl::label(GL_VERTEX_ARRAY, vao, "distortion vao");

  glCreateBuffers(2, buffer);

  gl::label(GL_BUFFER, vbo, "distortion vbo");
  glNamedBufferData(vbo, verts.size() * sizeof(vertex), verts.data(), GL_STATIC_DRAW);

  gl::label(GL_BUFFER, ibo, "distortion ibo");
  glNamedBufferData(ibo, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);

  for (int i = 0;i < 4;++i)
    glEnableVertexArrayAttrib(vao, i);
    
  glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, p));
  glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, r));
  glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, g));
  glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, b));

  for (int i = 0;i < 4;++i)
    glVertexArrayAttribBinding(vao, i, 0);

  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vertex)); // bind the data

  log("distortion")->info("{} vertices, {} indices", verts.size(), indices.size());
}

distortion::~distortion() {
  log("distortion")->info("shutting down");
  glDeleteBuffers(2, buffer);
  glDeleteVertexArrays(1, &vao);
}

void distortion::render(GLuint resolutionTexture) {
  glDisable(GL_DEPTH_TEST);
  glBindVertexArray(vao);
  glUseProgram(program.programId);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBindTexture(GL_TEXTURE_2D, resolutionTexture); // TODO: move this into a uniform so we can bake it into the program?
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glDrawElements(GL_TRIANGLES, n_indices, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  glEnable(GL_DEPTH_TEST);
}
