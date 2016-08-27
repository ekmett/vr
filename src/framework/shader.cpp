#include "framework/stdafx.h"
#include "framework/shader.h"
#include "framework/gl.h"

namespace framework {
  namespace gl {

    string shader_log(GLuint v) {
      GLint len;
      glGetShaderiv(v, GL_INFO_LOG_LENGTH, &len);
      std::string result;
      result.resize(len + 1);
      GLsizei actualLen;
      glGetShaderInfoLog(v, len + 1, &actualLen, const_cast<GLchar *>(result.c_str()));
      result.resize(actualLen);
      return result;
    }

    string program_log(GLuint p) {
      GLint len;
      glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
      std::string result;
      result.resize(len + 1);
      GLsizei actualLen;
      glGetProgramInfoLog(p, len + 1, &actualLen, const_cast<GLchar *>(result.c_str()));
      result.resize(actualLen);
      return result;
    }

    GLuint compile(const char * name, const char * vertexShader, const char * fragmentShader) {

      int programId = glCreateProgram();
      label(GL_PROGRAM, programId, "{} program", name);
      GLuint v = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(v, 1, &vertexShader, NULL);
      glCompileShader(v);

      label(GL_SHADER, v, "{} vertex shader", name);

      GLint vShaderCompiled = GL_FALSE;
      glGetShaderiv(v, GL_COMPILE_STATUS, &vShaderCompiled);

      if (vShaderCompiled != GL_TRUE) {
        glDeleteProgram(programId);
        string log = shader_log(v);
        glDeleteShader(v);
        die("error in vertex shader: {} ({})\n{}", name, v, log);
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
        string log = shader_log(f);
        glDeleteShader(f);
        die("error in fragment shader: {} ({})\n{}", name, f, log);
      }

      glAttachShader(programId, f);
      glDeleteShader(f); // the program hangs onto this once it's attached
      glLinkProgram(programId);
      GLint programSuccess = GL_TRUE;
      glGetProgramiv(programId, GL_LINK_STATUS, &programSuccess);
      if (programSuccess != GL_TRUE) {
        string log = program_log(programId);
        glDeleteProgram(programId);
        die("error linking program: {} ({}):\n{}", name, programId, log);
      }

      glUseProgram(programId);
      glUseProgram(0);

      return programId;
    }
  }
}