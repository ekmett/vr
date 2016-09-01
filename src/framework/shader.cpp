#include "framework/stdafx.h"
#include "framework/shader.h"
#include "framework/gl.h"
#include "framework/std.h"
#include <boost/filesystem/operations.hpp>
#include <iostream>
#include <fstream>
#include <cerrno>

using namespace boost::filesystem;

static const char * const searchpath = "/";

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
      glCompileShaderIncludeARB(v, 1, &searchpath, nullptr);
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

      glCompileShaderIncludeARB(f, 1, &searchpath, nullptr);

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

    // this is a version of glCreateShaderProgramv that names the shader and includes NamedStringARB support for #include directives
    GLuint compile(GLuint type, const char * name, const char * body) {
      auto shader = glCreateShader(type);
      label(GL_SHADER, shader, "{} shader", name);
      glShaderSource(shader, 1, &body, nullptr);
      glCompileShaderIncludeARB(shader, 1, &searchpath, nullptr);
      GLint vShaderCompiled = GL_FALSE;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &vShaderCompiled);
      if (vShaderCompiled != GL_TRUE) {
        string log = shader_log(shader);
        glDeleteShader(shader);
        die("error in program shader: {} ({})\n{}", name, shader, log);
      }
      int program = glCreateProgram();
      glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
      glAttachShader(program, shader);
      glLinkProgram(program);
      glDetachShader(program, shader);
      glDeleteShader(shader);
      GLint programSuccess = GL_TRUE;
      glGetProgramiv(program, GL_LINK_STATUS, &programSuccess);
      if (programSuccess != GL_TRUE) {
        string log = program_log(program);
        glDeleteProgram(program);
        die("error linking program: {} ({}):\n{}", name, program, log);
      }
      glUseProgram(program);
      glUseProgram(0);
      return program;
    }

    string read_file(path filename) {
      auto name = filename.native();
      std::ifstream in(name.c_str(), std::ios::in | std::ios::binary);
      if (in) {
         std::string contents;
         in.seekg(0, std::ios::end);
         contents.resize(in.tellg());
         in.seekg(0, std::ios::beg);
         in.read(&contents[0], contents.size());
         in.close();
         return(contents);
      }
      //string truncated_name(name.begin(), name.end());
      die("unable to read file"); // : {}", truncated_name);
    }

    void include(path real, path imaginary) {
      if (is_directory(real)) {
        log("gl")->info("recursing into {}", real.string());
        for (auto && entry : directory_iterator(real)) {
          include(entry.path(), path(imaginary).append(entry.path().filename().native()));
        }
      } else if (is_regular_file(real)) {
        log("gl")->info("include {} as {}", real.string(), imaginary.generic_string());
        string contents = read_file(real);
        string imaginary_name = imaginary.generic_string();
        glNamedStringARB(GL_SHADER_INCLUDE_ARB, imaginary_name.size(), imaginary_name.c_str(), contents.size(), contents.c_str());
      } else {
        log("gl")->warn("ignoring file {}", real.string());
      }
    }
  }
}