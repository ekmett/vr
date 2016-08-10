#pragma once

#include <openvr.h>
#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include "noncopyable.h"
#include <glm/glm.hpp>
#include "window.h"
#include "tracker.h"
#include "distortion.h"

struct app : noncopyable {
  app(sdl_window & window, openvr_tracker & tracker)
    : window(window)
    , tracker(tracker)
    , distortion(window, tracker) {}

  void run();
  void render(); // render a frame
  virtual ~app() {}

private:
  sdl_window & window;
  openvr_tracker & tracker;
  openvr_distortion distortion;
  rendermodel_shader rendermodel_shader;
  controller_shader controller_shader;
};

