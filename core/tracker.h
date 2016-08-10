#pragma once

#include <string>
#include <openvr.h>

#include "signal.h"
#include "log.h"
#include "noncopyable.h"


struct tracker : noncopyable {
  virtual ~tracker() {}
};

struct openvr_tracker : tracker {
  openvr_tracker(std::shared_ptr<spdlog::logger> & vr_log);
  virtual ~openvr_tracker();
  
  bool poll();

  bool focus_lost;
  bool dashboard_active;
  bool chaperone_bounds_visible;

  signal<void()> on_quit; // on a quit signal from the hmd or compositor termination
  signal<void()> on_ipd_changed;
  signal<void()> on_focus_captured;
  signal<void()> on_focus_released;
  signal<void()> on_dashboard_activated;
  signal<void()> on_dashboard_deactivated;
  signal<void()> on_chaperone_bounds_shown;
  signal<void()> on_chaperone_bounds_hidden;
  signal<void(uint64_t, uint64_t)> on_chaperone_universe_changed; 

  std::string device_string(vr::TrackedDeviceIndex_t device, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *error = nullptr);
  std::string model_name(int i);
  std::string component_name(std::string model, int i);
  std::string driver();
  std::string serial_number();
  vr::IVRSystem * operator -> () { return hmd; }
//private:
  void recalculate_chaperone_data();
  vr::IVRSystem * hmd;
  vr::IVRRenderModels * renderModels;
  vr::IVRCompositor * compositor;
  vr::IVRChaperone * chaperone;
  std::shared_ptr<spdlog::logger> log;
};