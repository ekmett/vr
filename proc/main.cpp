#include "stdafx.h"
#include "../framework/openvr.h"
#include "../framework/sdl.h"

using namespace framework;

int main(int argc, char ** argv) {
  spdlog::set_pattern("%a %b %m %Y %H:%M:%S.%e - %n %l: %v [thread %t]"); // close enough to the native notifications from openvr that the debug log is readable.
  openvr vr;
  sdl_gl_window window("proc", { 4, 5, gl::profile::core });
  return 0;
}