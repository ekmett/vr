#pragma once

#include "filename.h"

// raii, requires opengl
struct gl_logger : noncopyable {
  gl_logger(filename_t name);
  virtual ~gl_logger();

  std::shared_ptr<spdlog::logger> logger;
};