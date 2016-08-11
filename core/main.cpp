#include <string>
#include <stdexcept>
#include "window.h"
#include "tracker.h"
#include "app.h"
#include "log.h"

using namespace core;
using namespace std;
using namespace spdlog;

int main(int argc, char *argv[]) {
  spdlog::set_pattern("%a %b %m %Y %H:%M:%S.%e - %n %l: %v [thread %t]"); // close enough to the native notifications from openvr that the debug log is readable.
  
  auto vr_log = spdlog::create<squelched_sink_mt<default_sink>>("vr",3);
  auto gl_log = spdlog::create<squelched_sink_mt<default_sink>>("gl", 3);
  auto sdl_log = spdlog::create<squelched_sink_mt<default_sink>>("sdl", 3);
  auto app_log = spdlog::create<default_sink>("app");

#ifndef _DEBUG
  try {
#endif

    openvr_tracker tracker(vr_log);

#ifdef _DEBUG
    sdl_window window(sdl_log, string("core - ") + tracker.driver() + " - " + tracker.serial_number(), true);

    gl_logger logger(gl_log); // start logging to the msvc console.
#else
    sdl_window window(sdl_log, "core");
#endif

    app world(app_log, window, tracker);
    world.run();

    return 0;
#ifndef _DEBUG
  } catch (std::runtime_error & e) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "fatal error", e.what(), NULL);
    return 1;
  }
#endif
}