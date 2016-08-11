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
#include "rendermodel.h"
#include "distortion.h"

namespace core {
  struct app : noncopyable {
    app(std::shared_ptr<spdlog::logger> & log, sdl_window & window, openvr_tracker & tracker)
      : log(log)
      , window(window)
      , tracker(tracker)
      , distortion(window, tracker)
      , rendermodels(tracker)
      , window_on_quit_connection(window.on_quit.connect(on_quit))
      , tracker_on_quit_connection(tracker.on_quit.connect(on_quit)) {}

    void run();
    void render(); // render a frame
    virtual ~app() {}

    signal<void()> on_quit;

  private:
    std::shared_ptr<spdlog::logger> log;
    sdl_window & window;
    openvr_tracker & tracker;
    openvr_distortion distortion;
    rendermodel::manager rendermodels;
    controller_shader controller_shader;
    scoped_connection window_on_quit_connection, tracker_on_quit_connection;
  };
}
