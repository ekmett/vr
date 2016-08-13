#pragma once

#include <openvr.h>
#include "std.h"
#include "spdlog.h"
#include "signal.h"
#include "noncopyable.h"

#ifdef _WIN32
#pragma comment(lib, "openvr_api")
#endif

namespace framework {

  // construction is safely multi-threadedly re-entrant
  struct openvr : noncopyable {
    typedef uint64_t process_id;

    enum device_id : uint32_t {
      hmd = vr::k_unTrackedDeviceIndex_Hmd,
      max = vr::k_unMaxTrackedDeviceCount
    };

    openvr();
    virtual ~openvr();

    bool poll() const;

    vr::TrackedDeviceClass device_class(device_id i) const;
    bool valid_device(device_id index) const;
    string device_string(device_id index, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError * error = nullptr) const;    

    inline string driver() const;
    inline string serial_number() const;

    vr::IVRSystem * system;


    // static signals
    static signal<void(vr::VREvent_t &)> on_event;
    static signal<void()> on_process_quit, on_quit, on_ipd_changed, on_hide_render_models, on_show_render_models, on_model_skin_settings_have_changed, on_dashboard_activated, on_dashboard_deactivated, on_chaperone_bounds_shown, on_chaperone_bounds_hidden, on_chaperone_data_has_changed, on_chaperone_settings_have_changed, on_chaperone_temp_data_has_changed, on_tracked_device_activated, on_tracked_device_deactivated;
    static signal<void(process_id)> on_focus_enter, on_focus_leave, on_overlay_focus_changed, on_dashboard_thumb_selected, on_dashboard_requested;
    static signal<void(vr::VREvent_Controller_t &)> on_button_press, on_button_unpress, on_button_touch, on_button_untouch;
    static signal<void(vr::VREvent_Mouse_t &)> on_mouse_move, on_mouse_button_up, on_mouse_button_down, on_scroll, on_touchpad_move;
    static signal<void(vr::VREvent_Chaperone_t &)> on_chaperone_universe_changed;
    static signal<void(vr::VREvent_Process_t &)> on_scene_focus_lost, on_scene_focus_gained,
      on_scene_application_changed, on_scene_focus_changed, on_input_focus_changed, on_input_focus_captured, on_input_focus_released, on_scene_application_secondary_rendering_started;

    // static utilities
    static const char * show_tracked_device_class(vr::TrackedDeviceClass c);
    static const char * show_event_type(int e);

  };

  
  inline vr::TrackedDeviceClass openvr::device_class(device_id i) const {
    return system->GetTrackedDeviceClass(i);
  }

  inline bool openvr::valid_device(device_id index) const {
    return device_class(index) != vr::TrackedDeviceClass_Invalid;
  }

  inline string openvr::driver() const { 
    return device_string(hmd, vr::Prop_TrackingSystemName_String); 
  }

  inline string openvr::serial_number() const {
    return device_string(hmd, vr::Prop_TrackingSystemName_String);
  }

}