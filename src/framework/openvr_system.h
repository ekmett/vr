#pragma once

#include <openvr.h>
#include <chrono>

#ifdef _WIN32
#pragma comment(lib, "openvr_api")
#endif

#include "framework/glm.h"
#include "framework/std.h"
#include "framework/spdlog.h"
#include "framework/signal.h"
#include "framework/noncopyable.h"



namespace framework {

  namespace openvr {
    typedef uint64_t process_id;

    enum device_id : uint32_t {
      hmd = vr::k_unTrackedDeviceIndex_Hmd,
      max = vr::k_unMaxTrackedDeviceCount
    };

    static inline glm::mat4 hmd_mat4(const vr::HmdMatrix44_t & m) {
      return glm::make_mat4((float*)&m.m);
    }

    static inline glm::mat4 hmd_mat3x4(const vr::HmdMatrix34_t & m) {
      return glm::mat4(
        m.m[0][0], m.m[1][0], m.m[2][0], 0.0,
        m.m[0][1], m.m[1][1], m.m[2][1], 0.0,
        m.m[0][2], m.m[1][2], m.m[2][2], 0.0,
        m.m[0][3], m.m[1][3], m.m[2][3], 1.0f
      );
    }



    // construction is safely multi-threadedly re-entrant
    struct system : noncopyable {
      system();
      virtual ~system();

      bool poll() const;

      vr::TrackedDeviceClass device_class(device_id i) const;
      bool valid_device(device_id index) const;
      string device_string(device_id index, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError * error = nullptr) const;

      inline string driver() const;
      inline string driver_version() const;
      inline string serial_number() const;

      vr::IVRSystem * handle;

      // cached information
      float display_frequency;
      duration<float> frame_duration;
      duration<float> vsync_to_photons;

      inline duration<float> time_since_last_vsync() {
        float t;
        handle->GetTimeSinceLastVsync(&t, NULL);
        return duration<float>(t);
      }

      inline duration<float> time_to_photons() {
        return frame_duration - time_since_last_vsync() + vsync_to_photons;
      }

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
      const char * show_tracked_device_class(vr::TrackedDeviceClass c) const noexcept;
      const char * show_event_type(int e) const noexcept;
      const char * show_compositor_error(vr::EVRCompositorError e) const noexcept;
    };


    inline vr::TrackedDeviceClass system::device_class(device_id i) const {
      return handle->GetTrackedDeviceClass(i);
    }

    inline bool system::valid_device(device_id index) const {
      return device_class(index) != vr::TrackedDeviceClass_Invalid;
    }

    inline string system::driver() const {
      return device_string(hmd, vr::Prop_TrackingSystemName_String);
    }

    inline string system::driver_version() const {
      return device_string(hmd, vr::Prop_DriverVersion_String);
    }
    inline string system::serial_number() const {
      return device_string(hmd, vr::Prop_SerialNumber_String);
    }
  }
}