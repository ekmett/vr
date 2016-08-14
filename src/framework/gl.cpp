#include "stdafx.h"
#include "spdlog.h"
#include "gl.h"

namespace framework {
  namespace gl {
    static inline spdlog::level::level_enum log_severity(GLenum severity) {
      using namespace spdlog::level;
      switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: return critical;
        case GL_DEBUG_SEVERITY_MEDIUM: return warn;
        default: return info;
      }
    }

    static void APIENTRY gl_debugging_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
      log("gl")->log(log_severity(severity), "{} {}: {} ({})", show_debug_source(source), show_debug_message_type(type), message, id);
    }

    debugger::debugger() {
      glDebugMessageCallback((GLDEBUGPROC)gl_debugging_callback, nullptr);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_LOW, 5, "start");
    }

    debugger::~debugger() {
      glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_LOW, 4, "stop");
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
      glDebugMessageCallback(nullptr, nullptr);
    }

    const char * show_object_label_type(GLenum t) {
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

    const char * show_debug_source(GLenum s) {
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

    const char * gl::show_debug_message_type(GLenum t) {
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
  }
}