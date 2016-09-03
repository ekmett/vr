#pragma once

#include "framework/glew.h"
#include "framework/std.h"
#include "framework/noncopyable.h"
#include <boost/filesystem/path.hpp>

namespace framework {
  namespace gl {
    extern GLuint compile(const char * name); 
    extern GLuint compile(const char * name, const char * vertexShader, const char * fragmentShader); // build a standard vertex-shader fragment-shader pair
    extern GLuint compile(GLuint type, const char * name, const char * body); // build a shader program
    extern GLuint compile(GLuint type, const char * name); // build a shader program from file

    void include(boost::filesystem::path real, boost::filesystem::path imaginary = L"/");

    // use a simplistic shader compiler for now
    struct shader : noncopyable {
      shader(GLuint type, const char * name) : programId(compile(type, name)) {}
      shader(const char * name) : programId(compile(name)) {}
      shader(const char * name, const char * vertexShader, const char * fragmentShader) : programId(compile(name, vertexShader, fragmentShader)) {}
      shader(GLuint type, const char * name, const char * body) : programId(compile(type, name, body)) {}
      virtual ~shader() {
        if (programId != 0) glDeleteProgram(programId);
      }
      GLuint programId;
    };

    // this may eventually pick up more stuff
    struct compiler {
      template <typename ... T> compiler(T ... args) {
        include(args...);
      }
      ~compiler() {}
    };
  }
}