#pragma once

#include "gl.h"
#include "std.h"
#include "half.h"
#include "glm.h"
#include "openvr.h"

namespace framework {
  template<class R, class T> ptrdiff_t get_offset(R T::* mem) {
    return offsetof(T, *mem);
  }
  template <typename T> struct gl_array_info {
    typedef T element_type;
    static const int element_count = 1;
  };
  template <> struct gl_array_info<vec2> {
    typedef float element_type;
    static const int element_count = 2;
  };
  template <> struct gl_array_info<vr::HmdVector2_t> {
    typedef float element_type;
    static const int element_count = 3;
  };
  template <> struct gl_array_info<vec3> {
    typedef float element_type;
    static const int element_count = 3;
  };
  template <> struct gl_array_info<vr::HmdVector3_t> {
    typedef float element_type;
    static const int element_count = 3;
  };
  template <> struct gl_array_info<vec4> {
    typedef float element_type;
    static const int element_count = 4;
  };
  template <> struct gl_array_info<vr::HmdVector4_t> {
    typedef float element_type;
    static const int element_count = 4;
  };
  template <typename T, size_t N> struct gl_array_info<T[N]> {
    typedef T element_type;
    static const int element_count = N;
  };

  template <GLenum type>
  struct attrib {
    GLint size;
    bool normalized;
    GLuint relative_offset;
    void set(GLuint vao, int i) const {
      log("vao")->info("attrib: {}, type: {}{}, normalized: {}, offset: {}", i, gl::show_enum_type(type), size == 1 ? "" : fmt::format("[{}]", size), normalized, relative_offset);
      glEnableVertexArrayAttrib(vao, i);
      glVertexArrayAttribFormat(vao, i, size, type, normalized ? GL_TRUE : GL_FALSE, relative_offset);
      glVertexArrayAttribBinding(vao, i, 0);
    }
  };

  template <typename R, typename T> auto make_attrib(R T::* pm, bool normalized = false) {
    typedef gl_array_info<R> array_info;
    return attrib<gl::type_enum<typename array_info::element_type>::value> {
      array_info::element_count,
      normalized,
      GLuint(get_offset<R,T>(pm))
    };
  }

  template <typename T> auto self_attrib(bool normalized = false) {
    typedef gl_array_info<T> array_info;
    return attrib<gl::type_enum<typename array_info::element_type>::value> {
      array_info::element_count,
      normalized,
      0
    };
  }

  template <GLenum type>
  struct iattrib {
    GLint size;
    GLuint relative_offset;
    void set(GLuint vao, int i) const {
      log("vao")->info("iattrib: {}, type: {}{}, offset: {}", i, gl::show_enum_type(type), size == 1 ? "" : fmt::format("[{}]",size), relative_offset);
      glEnableVertexArrayAttrib(vao, i);
      glVertexArrayAttribIFormat(vao, i, size, type, relative_offset);
      glVertexArrayAttribBinding(vao, i, 0);
    }
  };

  template <typename R, typename T> auto make_iattrib(R T::* pm) {
    typedef gl_array_info<R> array_info;
    return iattrib<gl::type_enum<typename array_info::element_type>::value> {
      array_info::element_count,
      GLuint(get_offset<R, T>(pm))
    };
  }

  // should be a value template when we can use c++14
  template <typename T> auto self_iattrib() {
    typedef gl_array_info<T> array_info;
    return iattrib<gl::type_enum<typename array_info::element_type>::value> {
      array_info::element_count,
      0
    };
  }

// encapsulating this so we can eventually share these
  template <typename T, GLenum E = GL_UNSIGNED_SHORT>
  struct vertex_array {
    typedef T vertex_type;
    typedef typename gl::enum_type<E>::type index_type;
    static const GLenum index = E;
    template <typename ... Attribs>
    vertex_array(const string & name, bool element_array_buffer, Attribs && ... attribs ) : name(name) {
      log("vao")->info("creating vao {}", name);
      glCreateVertexArrays(1, &vao);
      gl::label(GL_VERTEX_ARRAY, vao, "{} vao", name);
      glCreateBuffers(1, &vbo);
      glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(T));
      gl::label(GL_BUFFER, vbo, "{} vertices", name);
      if (element_array_buffer) {
        glCreateBuffers(1, &ibo);
        gl::label(GL_BUFFER, ibo, "{} indices", name);
      } else ibo = 0;
      attributes(0, attribs ...);
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

    void load_elements(const index_type * indices, size_t count, GLenum usage = GL_STREAM_DRAW) const {
      if (!ibo) die("no element array buffer");
      glNamedBufferData(ibo, sizeof(index_type) * count, indices, usage);
      glVertexArrayElementBuffer(vao, ibo);
    }

    void load_elements(const std::vector<index_type> & v, GLenum usage = GL_STREAM_DRAW) const {
      load_elements(v.data(), v.size(), usage);
    }

    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    string name;
  };
}