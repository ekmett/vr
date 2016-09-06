#include "framework/stdafx.h"
#include "framework/glm.h"
#include "framework/std.h"
#include "framework/sdl_window.h"
#include "framework/openvr.h"
#include "distortion.h"

using namespace framework;
using namespace glm;
using namespace vr;

static float sign(vec2 p1, vec2 p2, vec2 p3) {
  return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

static bool point_in_triangle(vec2 p, vec2 * v) {
  bool b1, b2, b3;

  b1 = sign(p, v[0], v[1]) <= 0.0f;
  b2 = sign(p, v[1], v[2]) <= 0.0f;
  b3 = sign(p, v[2], v[0]) <= 0.0f;

  return (b1 == b2) && (b2 == b3);
}

static bool point_in_mesh(vec2 p, vec2 * mesh, int vertices) {
  for (int i = 0; i < vertices; i += 3) {
    if (point_in_triangle(p, mesh + i)) return true;
  }
  return false;
}

namespace framework {

  distortion::distortion(GLushort segmentsH, GLushort segmentsV) : mask("distortion_mask"), warp("distortion_warp") {   
    glUniformBlockBinding(warp.programId, 0, 0);

    float w = (float)(1.0 / float(segmentsH - 1)),
      h = (float)(1.0 / float(segmentsV - 1));

    struct vertex {
      vec2 p, r, g, b;
      GLushort eye;
    };

    vector<vec2> hidden_verts;
    for (int i = 0;i < 2;++i) {
      auto mesh = VRSystem()->GetHiddenAreaMesh(EVREye(i));
      if (mesh.unTriangleCount == 0) continue;
      for (int j = 0; j < mesh.unTriangleCount * 3; ++j) {
        auto v = mesh.pVertexData[j].v;
        hidden_verts.push_back(vec2(v[0] * 2 - 1, 1 - 2 * v[1]));
      }
      if (i == 0) n_hidden_left = int(hidden_verts.size());
    }
    glProgramUniform1i(mask.programId, 0, n_hidden_left);

    n_hidden = int(hidden_verts.size());


    auto want = [&](float v[2], int i) -> bool {
      vec2 p(v[0] * 2 - 1, 1 - 2 * v[1]);

      return 0.05 < std::min(v[0], v[1]) && std::max(v[0], v[1]) <= 0.95 &&
           ((i == 0) ? !point_in_mesh(p, hidden_verts.data(), n_hidden_left)
                     : !point_in_mesh(p, hidden_verts.data() + n_hidden_left, n_hidden - n_hidden_left))
        ;
    };

    vector<bool> good;
    for (int i = 0;i < 2;++i) {
      for (int y = 0; y < segmentsV; ++y) {
        for (int x = 0; x < segmentsH; ++x) {
          DistortionCoordinates_t dc = VRSystem()->ComputeDistortion(vr::EVREye(i), x*w, 1 - y*h);
          good.push_back(want(dc.rfGreen, i) || want(dc.rfBlue, i) || want(dc.rfRed, i));
        }
      }
    }

    vector<bool> keep = good;
    assert(keep.size() == good.size());

    // mark all the vertices we need
    for (int i = 0; i < 2; ++i) {
      GLushort offset = i*segmentsH*segmentsV;
      for (GLushort y = 0; y < segmentsV - 1; y++) {
        for (GLushort x = 0; x < segmentsH - 1; x++) {
          GLushort a = segmentsH*y + x + offset, b = a + 1, c = a + segmentsH, d = c + 1;
          GLushort dc = i == 0 ? d : c;
          GLushort ab = i == 0 ? a : b;
          if (good[a] || good[b] || good[dc]) keep[a] = keep[b] = keep[dc] = true; // if any corner is good the triangle is
          if (good[ab] || good[d] || good[c]) keep[ab] = keep[d] = keep[c] = true; // if any corner is good the triangle is
        }
      }
    }

    // a b   a b d & a d c
    // c d   a b c & b d c

    vector<vertex> verts;
    vector<int> ranks;
    {
      int j = 0, r = 0;
      for (GLushort i = 0;i < 2;++i) {
        float o = float(i) - 1; // -1 for left eye, 0 for right
        for (int y = 0; y < segmentsV; ++y) {
          for (int x = 0; x < segmentsH; ++x) {
            float u = x*w, v = 1 - y*h;
            ranks.push_back(r);
            if (keep[j++]) {
              DistortionCoordinates_t dc = VRSystem()->ComputeDistortion(vr::EVREye(i), u, v);
              verts.push_back(vertex{
                vec2(o + u, 2 * y*h - 1),
                vec2(dc.rfRed[0],   1 - dc.rfRed[1]),
                vec2(dc.rfGreen[0], 1 - dc.rfGreen[1]),
                vec2(dc.rfBlue[0],  1 - dc.rfBlue[1]),
                i
              });
              ++r;
            }
          }
        }
      }
    }

    vector<GLushort> indices;

    // mark all the vertices we need
    for (int i = 0; i < 2; ++i) {
      GLushort offset = i*segmentsH*segmentsV;
      for (GLushort y = 0; y < segmentsV - 1; y++) {
        for (GLushort x = 0; x < segmentsH - 1; x++) {
          GLushort a = segmentsH*y + x + offset, b = a + 1, c = a + segmentsH, d = c + 1;
          GLushort ra = ranks[a], rb = ranks[b], rc = ranks[c], rd = ranks[d];
          GLushort ab = i == 0 ? a : b;
          GLushort dc = i == 0 ? d : c;
          GLushort rab = i == 0 ? ra : rb;
          GLushort rdc = i == 0 ? rd : rc;
          if (keep[a] && keep[b] && keep[dc]) {
            indices.push_back(ra);
            indices.push_back(rb);
            indices.push_back(rdc);
          }
          if (keep[ab] && keep[d] && keep[c]) {
            indices.push_back(rab);
            indices.push_back(rd);
            indices.push_back(rc);
          }
        }
      }
    }

    n_indices = int(indices.size());

    glCreateVertexArrays(2, array);
    gl::label(GL_VERTEX_ARRAY, vao, "distortion vao");
    gl::label(GL_VERTEX_ARRAY, hidden_vao, "distortion hidden vao");

    glCreateBuffers(3, buffer);

    gl::label(GL_BUFFER, vbo, "distortion vbo");
    glNamedBufferData(vbo, verts.size() * sizeof(vertex), verts.data(), GL_STATIC_DRAW);

    gl::label(GL_BUFFER, hidden_vbo, "distortion hidden vbo");
    glNamedBufferData(hidden_vbo, hidden_verts.size() * sizeof(vec2), hidden_verts.data(), GL_STATIC_DRAW);

    gl::label(GL_BUFFER, ibo, "distortion ibo");
    glNamedBufferData(ibo, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);

    for (int i = 0;i < 5;++i) {
      glEnableVertexArrayAttrib(vao, i);
      glVertexArrayAttribBinding(vao, i, 0);
    }

    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, p));
    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, r));
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, g));
    glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, b));
    glVertexArrayAttribIFormat(vao, 4, 1, GL_UNSIGNED_SHORT, offsetof(vertex, eye));

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vertex)); // bind the data
    glVertexArrayElementBuffer(vao, ibo);

    glEnableVertexArrayAttrib(hidden_vao, 0);
    glVertexArrayAttribFormat(hidden_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(hidden_vao, 0, 0);
    glVertexArrayVertexBuffer(hidden_vao, 0, hidden_vbo, 0, sizeof(vec2));

    log("distortion")->info("{} vertices, {} indices, {} hidden triangles", verts.size(), indices.size(), hidden_verts.size() / 3);
  }

  distortion::~distortion() {
    glDeleteBuffers(3, buffer);
    glDeleteVertexArrays(2, array);
  }

  void distortion::render_stencil() {
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilMask(1);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glBindVertexArray(hidden_vao);
    glUseProgram(mask.programId);
    if (debug_wireframe_render_stencil) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, n_hidden); // put 1 in the stencil mask everywhere the hidden mesh lies
    if (debug_wireframe_render_stencil) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glStencilFunc(GL_EQUAL, 0, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // use the stencil mask to disable writes
    glStencilMask(0);
    //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glUseProgram(0);
    glBindVertexArray(0);
  }

  void distortion::render(int view_mask) {
    log("distortion")->info("rendering {}", view_mask);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(1);
    glBindVertexArray(vao);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glUseProgram(warp.programId);
    if (debug_wireframe_render) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, n_indices, GL_UNSIGNED_SHORT, nullptr);
    if (debug_wireframe_render) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    log("distortion")->info("rendering complete");
  }

}