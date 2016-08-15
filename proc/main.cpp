#include "stdafx.h"
#include "app.h"

using namespace proc;

int SDL_main(int argc, char ** argv) {
  spdlog::set_pattern("%a %b %m %Y %H:%M:%S.%e - %n %l: %v [thread %t]"); // close enough to the native notifications from openvr that the debug log is readable.
  cds_main_thread_attachment<> main_thread;

  app main;
  main.run();

  spdlog::details::registry::instance().drop_all(); // clean up. ideally i'd tell them all to flush in case there is a dangling reference to them
  return 0;
}