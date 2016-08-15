#pragma once

#include "framework.h"
#include <random>

namespace proc {

  using namespace framework;
  using namespace concurrency;

  struct app {
    app() : window("proc", { 4, 5, gl::profile::core }) {}
    ~app() {}
    void run();

    openvr::system vr;
    sdl::window window;
    std::mt19937 rng; // for the main thread
  };

}