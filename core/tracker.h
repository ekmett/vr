#pragma once

#include <string>
#include <openvr.h>

#include "signal.h"
#include "log.h"
#include "noncopyable.h"

namespace core {
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

    // buttons
    signal<void(vr::VREvent_Controller_t &)> on_button_press, on_button_unpress, on_button_touch, on_button_untouch;

    // mouse events
    signal<void(vr::VREvent_Mouse_t &)> on_mouse_move, on_mouse_button_up, on_mouse_button_down, on_scroll, on_touchpad_move;

    // focus
    signal<void()> on_dashboard_activated, on_dashboard_deactivated, on_chaperone_bounds_shown, on_chaperone_bounds_hidden;

    signal<void(uint64_t)> on_focus_enter, on_focus_leave, on_overlay_focus_changed, on_dashboard_thumb_selected, on_dashboard_requested; // overlays

    // chaperone
    signal<void(vr::VREvent_Chaperone_t &)> on_chaperone_universe_changed;
    signal<void()> on_chaperone_data_has_changed, on_chaperone_settings_have_changed, on_chaperone_temp_data_has_changed;
    signal<void()> on_tracked_device_activated, on_tracked_device_deactivated;
    // process
    signal<void(vr::VREvent_Process_t &)> on_scene_focus_lost, on_scene_focus_gained, 
        on_scene_application_changed, on_scene_focus_changed, on_input_focus_changed, on_input_focus_captured, on_input_focus_released, on_scene_application_secondary_rendering_started;
   
    signal<void()> on_hide_render_models, on_show_render_models, on_model_skin_settings_have_changed;



    bool valid_device(int i) { return device_class(i) != vr::TrackedDeviceClass_Invalid; }
    vr::TrackedDeviceClass device_class(int i) { return hmd->GetTrackedDeviceClass(i); }
    std::string device_string(vr::TrackedDeviceIndex_t device, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *error = nullptr);
    std::string model_name(int i);
    int model_count() { return renderModels->GetRenderModelCount(); }
    std::string component_name(std::string model, int i);
    std::string component_model(std::string model, std::string cname);
    inline int component_count(std::string model) { return renderModels->GetComponentCount(model.c_str()); }
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
    vr::VRControllerState_t controller_state[vr::k_unMaxTrackedDeviceCount];
  };
}