#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <openvr.h>
#include "util.h"
#include "shader.h"
#include <glm/glm.hpp>

using namespace glm;
using namespace vr;


shader::shader(const char * name, const char * vertexShader, const char * fragmentShader) 
: programId(glCreateProgram()) {
  objectLabelf(GL_PROGRAM, programId, "%s program", name);
  GLuint v = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(v, 1, &vertexShader, NULL);
  glCompileShader(v);

  objectLabelf(GL_SHADER, v, "%s vertex shader", name);

  GLint vShaderCompiled = GL_FALSE;
  glGetShaderiv(v, GL_COMPILE_STATUS, &vShaderCompiled);

  if (vShaderCompiled != GL_TRUE) {
    glDeleteProgram(programId);
    glDeleteShader(v);
    die("%s - Unable to compile vertex shader %d!\n", name, v);
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
    die("%s - Unable to compile fragment shader %d!\n", name, f);
  }

  glAttachShader(programId, f);
  glDeleteShader(f); // the program hangs onto this once it's attached

  glLinkProgram(programId);

  GLint programSuccess = GL_TRUE;
  glGetProgramiv(programId, GL_LINK_STATUS, &programSuccess);
  if (programSuccess != GL_TRUE) {
    glDeleteProgram(programId);
    die("%s - Error linking program %d!\n", name, programId);
  }

  glUseProgram(programId);
  glUseProgram(0);
}

shader::~shader() {
  glDeleteProgram(programId);
}

rendermodel_shader::rendermodel_shader() : shader("model",
    R"(#version 410
		   uniform mat4 matrix;
		   layout(location = 0) in vec4 position;
		   layout(location = 1) in vec3 v3NormalIn;
		   layout(location = 2) in vec2 v2TexCoordsIn;
		   out vec2 v2TexCoord;
		   void main() {
		     v2TexCoord = v2TexCoordsIn;
		     gl_Position = matrix * vec4(position.xyz, 1);
		   })",
    R"(#version 410 core
		   uniform sampler2D diffuse;
		   in vec2 v2TexCoord;
		   out vec4 outputColor;
		   void main() {
		     outputColor = texture( diffuse, v2TexCoord);
		   })") {}

controller_shader::controller_shader() : shader("controller",
    R"(#version 410    
       uniform mat4 matrix;
       layout(location = 0) in vec4 position;
       layout(location = 1) in vec3 v3ColorIn;
       out vec4 v4Color;
       void main() {
         v4Color.xyz = v3ColorIn; v4Color.a = 1.0;
         gl_Position = matrix * position;
       })",
    R"(#version 410
       in vec4 v4Color;
       out vec4 outputColor;
       void main() {
         outputColor = v4Color;
       })") {}