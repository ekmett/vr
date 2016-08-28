#pragma once

#include "framework/glew.h"
#include "framework/noncopyable.h"
#include <boost/filesystem/path.hpp>

namespace framework {
  namespace gl {
    extern GLuint compile(const char * name, const char * vertexShader, const char * fragmentShader);

    void include(boost::filesystem::path real, boost::filesystem::path imaginary = L"");

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