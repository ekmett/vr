#include <stdarg.h>
#include <stdio.h>
#include <string>
#include "util.h"
#include "shader.h"

using namespace util;

shader::shader(const char * name, const char * vertexShader, const char * fragmentShader) 
: programId(glCreateProgram()) {
  util::objectLabelf(GL_PROGRAM, programId, "%s program", name);
  GLuint v = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(v, 1, &vertexShader, NULL);
  glCompileShader(v);

  util::objectLabelf(GL_SHADER, v, "%s vertex shader", name);

  GLint vShaderCompiled = GL_FALSE;
  glGetShaderiv(v, GL_COMPILE_STATUS, &vShaderCompiled);

  if (vShaderCompiled != GL_TRUE) {
    glDeleteProgram(programId);
    glDeleteShader(v);
    die(__FUNCTION__, "%s - Unable to compile vertex shader %d!\n", name, v);
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
    die(__FUNCTION__, "%s - Unable to compile fragment shader %d!\n", name, f);
  }

  glAttachShader(programId, f);
  glDeleteShader(f); // the program hangs onto this once it's attached

  glLinkProgram(programId);

  GLint programSuccess = GL_TRUE;
  glGetProgramiv(programId, GL_LINK_STATUS, &programSuccess);
  if (programSuccess != GL_TRUE) {
    glDeleteProgram(programId);
    die(__FUNCTION__,"%s - Error linking program %d!\n", name, programId);
  }

  glUseProgram(programId);
  glUseProgram(0);
}

shader::~shader() {
  glDeleteProgram(programId);
}
