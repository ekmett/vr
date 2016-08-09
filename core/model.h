#pragma once

#include <string>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <openvr.h>
#include "noncopyable.h"

struct model : noncopyable {
  model(std::string & name, vr::RenderModel_t & vrModel, vr::RenderModel_TextureMap_t & vrDiffuse);  
  ~model();
  void draw();
private:
  GLuint vertBuffer;
  GLuint indexBuffer;
  GLuint vertArray;
  GLuint texture;
  GLsizei vertexCount;
  std::string name;
};