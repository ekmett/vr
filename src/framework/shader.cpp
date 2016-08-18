#include "framework/stdafx.h"
#include "framework/shader.h"
#include "framework/gl.h"

namespace framework {
  namespace gl {
    shader::shader(const char * name, const char * vertexShader, const char * fragmentShader)
      : programId(glCreateProgram()) {
      label(GL_PROGRAM, programId, "{} program", name);
      GLuint v = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(v, 1, &vertexShader, NULL);
      glCompileShader(v);

      label(GL_SHADER, v, "{} vertex shader", name);

      GLint vShaderCompiled = GL_FALSE;
      glGetShaderiv(v, GL_COMPILE_STATUS, &vShaderCompiled);

      if (vShaderCompiled != GL_TRUE) {
        glDeleteProgram(programId);
        glDeleteShader(v);
        die("{} - Unable to compile vertex shader {}!\n", name, v);
      }
      glAttachShader(programId, v);
      glDeleteShader(v); // the program hangs onto this once it's attached

      GLuint  f = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(f, 1, &fragmentShader, NULL);
      glCompileShader(f);

      GLint fShaderCompiled = GL_FALSE;
      glGetShaderiv(f, GL_COMPILE_STATUS, &fShaderCompiled);
      if (fShaderCompiled != GL_TRUE) {
        glDeleteProgram(programId);
        glDeleteShader(f);
        die("{} - Unable to compile fragment shader {}!\n", name, f);
      }

      glAttachShader(programId, f);
      glDeleteShader(f); // the program hangs onto this once it's attached

      glLinkProgram(programId);

      GLint programSuccess = GL_TRUE;
      glGetProgramiv(programId, GL_LINK_STATUS, &programSuccess);
      if (programSuccess != GL_TRUE) {
        glDeleteProgram(programId);
        die("{} - Error linking program {}!\n", name, programId);
      }

      glUseProgram(programId);
      glUseProgram(0);
    }

    shader::~shader() {
      glDeleteProgram(programId);
    }
  }
}