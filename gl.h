#pragma once

#include "config.h"

#ifdef FRAMEWORK_SUPPORTS_OPENGL

#include "glew.h"
#include "noncopyable.h"

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

    template <typename ... Ts> inline void label(GLenum id, GLuint name, const char * format, const Ts & ... args) noexcept {
      string label = fmt::format(format, args...);
      glObjectLabel(id, name, (GLsizei) label.length(), label.c_str());
      // log("gl")->info("{} {}: {}", show_object_label_type(id), name, label);
    }
    void label(GLenum id, GLuint name, const char * label) noexcept;

    string get_label(GLenum id, GLuint name) noexcept;


    void check_framebuffer(GLuint framebuffer, GLenum role);

    struct debug_group : noncopyable {
      template <typename ... Ts> debug_group(int id, const char * format, const Ts & ... args) noexcept {
        string label = fmt::format(format, args...);
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, GLsizei(label.length()), label.c_str());
      }
      template <typename ... Ts> debug_group(const char * format, const Ts & ... args) noexcept {
        string label = fmt::format(format, args...);
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, GLsizei(label.length()), label.c_str());
      }
      debug_group(const char * s) noexcept {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, GLsizei(strlen(s)), s);
      }
      debug_group(int id, const char * s) noexcept {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, GLsizei(strlen(s)), s);
      }
      debug_group(const string & s) noexcept {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, GLsizei(s.length()), s.c_str());
      }
      debug_group(int id, string & s) noexcept {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, GLsizei(s.length()), s.c_str());
      }
      ~debug_group(){
        glPopDebugGroup();
      }
    };
  }
}

#endif
