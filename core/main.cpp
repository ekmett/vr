#include <string>
#include "window.h"
#include "tracker.h"
#include "app.h"
#include "filename.h"
#include "log.h"

#define SPDLOG_WCHAR_FILENAMES
#include <spdlog/spdlog.h>

using namespace std;
using namespace spdlog;

int main(int argc, char *argv[]) {
  openvr_tracker tracker;
  sdl_window window(string("core - ") + tracker.driver() + " - " + tracker.serial_number(), true);
  
  // ensure our app_data directory exists.
  windows_shell shell;
  filename_t home = shell.app_data() + L"\\digitigrade";
  _wmkdir(home.c_str());
  gl_logger logger(home + L"\\gl.log");
 
  app world(window, tracker);
  return 0;
}