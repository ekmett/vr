#pragma once

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

    const char * show_object_label_type(GLenum t);
    const char * show_debug_source(GLenum s);
    const char * show_debug_message_type(GLenum t);

    // raii, requires opengl
    struct debugger : noncopyable {
      debugger();
      virtual ~debugger();
    };
  }
}
