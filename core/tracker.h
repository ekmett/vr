#pragma once

#include <string>
#include <openvr.h>

#include "noncopyable.h"

struct tracker : noncopyable {
  virtual ~tracker() {}
};

struct openvr_tracker : tracker {
  openvr_tracker();
  virtual ~openvr_tracker();

  std::string driver();
  std::string serial_number();

  vr::IVRSystem * hmd;
  vr::IVRRenderModels * renderModels;
};