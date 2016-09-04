#pragma once

#include "framework/config.h"

#ifdef FRAMEWORK_SUPPORTS_OPENGL

#include "framework/glew.h"
#include "framework/noncopyable.h"

namespace framework {
  namespace gl {
    enum profile : int {
      core = 0, compatibility = 1, es = 2
    };
    struct version {
      int major, minor;
      profile profile;
    };

    const char * show_object_label_type(GLenum t) noexcept;
    const char * show_debug_source(GLenum s) noexcept;
    const char * show_debug_message_type(GLenum t) noexcept;
    const char * show_framebuffer_status_result(GLenum e) noexcept;

    // raii, requires opengl
    struct debugger : noncopyable {
      debugger() noexcept;
      virtual ~debugger() noexcept;
    };

    template <typename ... Args> inline void label(GLenum id, GLuint name, const char * format, const Args & ... args) noexcept {
      string label = fmt::format(format, args...);
      glObjectLabel(id, name, (GLsizei) label.length(), label.c_str());
      log("gl")->info("{} {}: {}", show_object_label_type(id), name, label);

    }
    void label(GLenum id, GLuint name, const char * label) noexcept;

    string get_label(GLenum id, GLuint name) noexcept;


    void check_framebuffer(GLuint framebuffer, GLenum role);

  }
}

#endif