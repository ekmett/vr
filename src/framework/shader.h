#pragma once

#include "framework/glew.h"
#include "framework/noncopyable.h"

namespace framework {
  namespace gl {
    // use a simplistic shader compiler for now
    struct shader : noncopyable {
      shader(const char * name, const char * vertexShader, const char * fragmentShader);
      virtual ~shader();
      GLuint programId;
    };
  }
}