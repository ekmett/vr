#pragma once

#include "framework/std.h"
#include "framework/openvr.h"

using namespace framework;

// assumes vr
struct overlay {
  overlay(string key, string name, int width, int height);
  ~overlay();
  vr::VROverlayHandle_t handle;
  vr::VROverlayHandle_t thumbnail_handle;
};