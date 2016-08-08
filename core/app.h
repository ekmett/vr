#pragma once

#include <openvr.h>
#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include "noncopyable.h"

struct eyes {
  GLuint depthBufferId[2];
  GLuint renderTextureId[2];
  GLuint renderFramebufferId[2];
  GLuint resolveTextureId[2];
  GLuint resolveFramebufferId[2];
};

class app : noncopyable {
public:
  static int run(int argc, char ** argv);
private:
  app(
    vr::IVRSystem & hmd,
    vr::IVRRenderModels & renderModels,
    int windowWidth,
    int windowHeight,
    SDL_Window * window,
    eyes eyes
  ) : hmd(hmd),
      renderModels(renderModels),
      windowWidth(windowWidth),
      windowHeight(windowHeight),
      window(window),
      eyes(eyes) {
  }
  virtual ~app();
  private:
    int windowWidth, windowHeight;
    vr::IVRSystem & hmd;
    vr::IVRRenderModels & renderModels;
    SDL_Window * window;
    eyes eyes; // left and right
};