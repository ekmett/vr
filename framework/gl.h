#pragma once

#include <GL/glew.h>
#include "noncopyable.h"

namespace framework {
  namespace gl {
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
