#include "framework.h"

#include <SDL.h>
using namespace framework;

int SDL_main(int argc, char ** argv) {
  spdlog::set_pattern("%a %b %m %Y %H:%M:%S.%e - %n %l: %v [thread %t]"); // close enough to the native notifications from openvr that the debug log is readable.
  cds_main_thread_attachment<> main_thread;
  
  openvr::system vr;

  sdl::subsystem video;
  sdl::gl_window window("proc", { 4, 5, gl::profile::core });

  while (!vr.poll() && !video.poll()) {



  }

  spdlog::details::registry::instance().drop_all();
 
  return 0;
}