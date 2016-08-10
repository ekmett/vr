#pragma once

#include <windows.h>
#include <ShlObj.h>

#define SPDLOG_WCHAR_FILENAMES
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/null_sink.h>

#include "util.h"
#include "noncopyable.h"

#ifdef _DEBUG
typedef spdlog::sinks::msvc_sink_mt default_sink;
#else
typedef spdlog::sinks::null_sink_mt default_sink;
#endif

using spdlog::filename_t;

struct shell : noncopyable {
  shell() {}
  virtual ~shell() {}
  virtual filename_t app_data() = 0;
};

struct windows_shell : shell {
  windows_shell() {}
  virtual ~windows_shell() {}
  filename_t app_data() {
    PWSTR userAppData = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &userAppData) != S_OK)
      die("Unable to locate application data folder");
    return userAppData; // implicit conversion
  }
};

// raii, requires opengl
struct gl_logger : noncopyable {
  gl_logger(std::shared_ptr<spdlog::logger> & logger);
  virtual ~gl_logger();

  std::shared_ptr<spdlog::logger> logger;
};

