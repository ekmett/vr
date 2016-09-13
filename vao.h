#pragma once

#include "gl.h"
#include "std.h"

namespace framework {
  // 3, GL_FLOAT, GL_FALSE, offsetof(vr::RenderModel_Vertex_t, vPosition)
  struct attrib {
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLuint relative_offset;
    void set(GLuint vao, int i) const {
      log("vao")->info("attrib: {}, size: {}", i, size);
      glEnableVertexArrayAttrib(vao, i);
      glVertexArrayAttribFormat(vao, i, size, type, normalized, relative_offset);
      glVertexArrayAttribBinding(vao, i, 0);
    }
  };

  struct iattrib {
    GLint size;
    GLenum type;
    GLuint relative_offset;
    void set(GLuint vao, int i) const {
      log("vao")->info("iattrib: {}, size: {}", i, size);
      glEnableVertexArrayAttrib(vao, i);
      glVertexArrayAttribIFormat(vao, i, size, type, relative_offset);
      glVertexArrayAttribBinding(vao, i, 0);
    }
  };

// encapsulating this so we can eventually share these
  template <typename T>
  struct vertex_array {
    template <typename ... Attribs>
    vertex_array(const string & name, bool element_array_buffer, Attribs && ... attribs ) : name(name) {
      log("vao")->info("creating vao {}", name);
      glCreateVertexArrays(1, &vao);
      gl::label(GL_VERTEX_ARRAY, vao, "{} vao", name);
      glCreateBuffers(1, &vbo);
      gl::label(GL_BUFFER, vbo, "{} vertices", name);
      if (element_array_buffer) {
        glCreateBuffers(1, &ibo);
        gl::label(GL_BUFFER, ibo, "{} indices", name);
        glVertexArrayElementBuffer(vao, ibo);
      } else ibo = 0;
      attributes(0, attribs ...);
      glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(T));
    }
    ~vertex_array() {
      if (ibo) {
        glDeleteBuffers(1, &ibo);
        ibo = 0;
      }
      glDeleteBuffers(1, &vbo);
      glDeleteVertexArrays(1, &vao);
    }
    operator GLuint () const { return vao; }
    void bind() const { glBindVertexArray(vao); }
    template <typename T, typename ... Ts> void attributes(int i, T first, Ts ... rest) {
      first.set(vao, i);
      attributes(i + 1, rest ...);
    }
    void attributes(int i) {
      log("vao")->info("{} {}", i, plural<string>(i, "attribute"));
    }

    void load(const T * v, size_t count, GLenum usage = GL_STREAM_DRAW) const {
      glNamedBufferData(vbo, sizeof(T) * count, v, usage);
    }

    void load(const std::vector<T> & v, GLenum usage = GL_STREAM_DRAW) const {
      load(v.data(), v.size(), usage);
    }

    void load_elements(const GLushort * indices, size_t count, GLenum usage = GL_STREAM_DRAW) const {
      if (!ibo) die("no element array buffer");
      glNamedBufferData(ibo, sizeof(GLushort) * count, indices, usage);
    }

    void load_elements(const std::vector<T> & v, GLenum usage = GL_STREAM_DRAW) const {
      load_elements(v.data(), v.size(), usage);
    }

    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    string name;
  };
}