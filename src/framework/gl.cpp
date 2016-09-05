#include "framework/stdafx.h"
#include "framework/spdlog.h"
#include "framework/error.h"
#include "framework/gl.h"

namespace framework {
  namespace gl {
    static inline spdlog::level::level_enum log_severity(GLenum severity) noexcept {
      using namespace spdlog::level;
      switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: return critical;
        case GL_DEBUG_SEVERITY_MEDIUM: return warn;
        default: return info;
      }
    }

    string trim(const char * message) {
      string result = message;
      int len = result.length();
      if (len > 0 && result[len - 1] == '\n')
        result.resize(len - 1);      
      return result;
    }

    static void APIENTRY gl_debugging_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    //  if (type != GL_DEBUG_TYPE_OTHER) 
        log("gl")->log(log_severity(severity), "{} {}: {} ({})", show_debug_source(source), show_debug_message_type(type), trim(message), id);
    }

    debugger::debugger() noexcept {
      glDebugMessageCallback((GLDEBUGPROC)gl_debugging_callback, nullptr);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_LOW, 5, "start");
    }

    debugger::~debugger() noexcept {
      glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_LOW, 4, "stop");
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
      glDebugMessageCallback(nullptr, nullptr);
    }

    void objectLabel(GLenum id, GLuint name, const char *fmt, ...) {
      va_list args;
      char buffer[2048];

      va_start(args, fmt);
      vsprintf_s(buffer, fmt, args);
      va_end(args);

      buffer[2047] = 0;
      glObjectLabel(id, name, static_cast<GLsizei>(strnlen_s(buffer, 2048)), buffer);
      auto gl_log = spdlog::get("gl");
      if (gl_log) {
      }
    }


    const char * show_object_label_type(GLenum t) noexcept {
      switch (t) {
        case GL_BUFFER: return "buffer";
        case GL_SHADER: return "shader";
        case GL_PROGRAM: return "program";
        case GL_VERTEX_ARRAY: return "vao";
        case GL_QUERY: return "query";
        case GL_PROGRAM_PIPELINE: return "program pipeline";
        case GL_TRANSFORM_FEEDBACK: return "transform feedback";
        case GL_SAMPLER: return "sampler";
        case GL_TEXTURE: return "texture";
        case GL_RENDERBUFFER: return "renderbuffer";
        case GL_FRAMEBUFFER: return "framebuffer";
        default: return "unknown";
      }
    }

    const char * show_debug_source(GLenum s) noexcept {
      switch (s) {
        case GL_DEBUG_SOURCE_API: return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "window system";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "third party";
        case GL_DEBUG_SOURCE_APPLICATION: return "application";
        case GL_DEBUG_SOURCE_OTHER: return "other";
        default: return "unknown";
      }
    }

    const char * gl::show_debug_message_type(GLenum t) noexcept {
      switch (t) {
        case GL_DEBUG_TYPE_ERROR: return "error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "undefined behavior";
        case GL_DEBUG_TYPE_PORTABILITY: return "portability";
        case GL_DEBUG_TYPE_PERFORMANCE: return "performance";
        case GL_DEBUG_TYPE_MARKER: return "marker";
        case GL_DEBUG_TYPE_PUSH_GROUP: return "push group";
        case GL_DEBUG_TYPE_POP_GROUP: return "pop group";
        case GL_DEBUG_TYPE_OTHER: return "other";
        default: return "unknown";
      }
    }
    void label(GLenum id, GLuint name, const char * label) noexcept {
      glObjectLabel(id, name, (GLsizei)strlen(label), label);
      log("gl")->info("{} {}: {}", show_object_label_type(id), name, label);
    }
    string get_label(GLenum id, GLuint name) noexcept {
      return fmt::format("{}", name);
      /*
      GLsizei len;
      glGetObjectLabel(id, name, 0, &len, nullptr);
      string label;
      if (!len) return label;
      label.resize(len + 1);
      GLsizei final_len;
      glGetObjectLabel(id, name, len, &final_len, const_cast<GLchar*>(label.c_str()));
      label.resize(len);
      return label;
      */
    }
    const char * show_framebuffer_status_result(GLenum e) noexcept {
      switch (e) {
        case GL_FRAMEBUFFER_COMPLETE: return "complete";
        case GL_FRAMEBUFFER_UNDEFINED: return "undefined";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "incomplete attachment";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "incomplete missing attachment";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "incomplete draw buffer";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "incomplete read buffer";
        case GL_FRAMEBUFFER_UNSUPPORTED: return "unsupported";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "incomplete multisample";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "incomplete layer targets";
        default: return "unknown";
      }
    }
    void check_framebuffer(GLuint framebuffer, GLenum role) {
      GLenum result = glCheckNamedFramebufferStatus(framebuffer, role);
      if (result != GL_FRAMEBUFFER_COMPLETE)
        die("bad framebuffer: {} ({}): {}", gl::get_label(GL_FRAMEBUFFER, framebuffer), framebuffer, show_framebuffer_status_result(result));
    }
  }
}