#pragma once

#include "framework/glew.h"
#include "framework/noncopyable.h"

namespace framework {
  namespace gl {
    extern GLuint compile(const char * name, const char * vertexShader, const char * fragmentShader);

    // use a simplistic shader compiler for now
    struct shader : noncopyable {
      shader(const char * name, const char * vertexShader, const char * fragmentShader) : programId(compile(name, vertexShader, fragmentShader)) {}
      virtual ~shader() {
        if (programId != 0) glDeleteProgram(programId);
      }
      GLuint programId;
    };
  }
}