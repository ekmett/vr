#include "framework/stdafx.h"
#include "framework/openvr_system.h"

#ifdef FRAMEWORK_SUPPORTS_OPENVR

#include "framework/std.h"
#include "framework/error.h"
#include <chrono>

using namespace vr;

namespace framework {
  namespace openvr {
    namespace detail {
      template<typename... Ts, typename f, typename err> static inline string buffered_with_error(f fun, err *e, Ts... args) {
        string result;
        uint32_t newlen = fun(args..., nullptr, 0, e), len=0;
        do {
          len = newlen;
          if (len <= 1) return "";
          result.resize(len);
          uint32_t newlen = fun(args..., const_cast<char*>(result.c_str()), len, e);
        } while (len != newlen);
        result.resize(newlen - 1); // we don't own the \0 at the end of a string.
        return result;
      }

      template<typename... Ts, typename f> static inline string buffered(f fun, Ts... args) {
        string result;        
        uint32_t newlen = fun(args..., nullptr, 0, e), len=0;
        do {
          len = newlen;
          if (len <= 1) return "";
          result.resize(len);
          uint32_t newlen = fun(args..., const_cast<char*>(result.c_str()), len);
        } while (len != newlen);
        result.resize(newlen - 1); // we don't own the \0 at the end of a string.
        return result;
      }
    }

    mutex openvr_init_mutex;
    static uint64_t openvr_initializations = 0; // guarded by openvr_init_mutex

    system::system() {
      lock_guard<mutex> guard(openvr_init_mutex);
      if (!openvr_initializations++) {
        // Initialize OpenVR
        auto eError = VRInitError_None;
        handle = VR_Init(&eError, VRApplication_Scene);
        if (eError != VRInitError_None)
          die("Unable to initialize OpenVR.\n{}", VR_GetVRInitErrorAsEnglishDescription(eError));
        log("vr")->info("initialized. driver: {} ({}) serial#: {}", driver(), driver_version(), serial_number());

        // cache some information about the display
        display_frequency = handle->GetFloatTrackedDeviceProperty(hmd, vr::Prop_DisplayFrequency_Float);
        frame_duration = duration<float> (1.f / display_frequency);
        vsync_to_photons = duration<float>(handle->GetFloatTrackedDeviceProperty(hmd, vr::Prop_SecondsFromVsyncToPhotons_Float));
        log("vr")->info("display frequency: {}, time from vsync to photons: {}", display_frequency, duration<float,std::milli>(vsync_to_photons));
      } else {
        handle = VRSystem();
        log("vr")->info("re-entrant initialization {}", openvr_initializations);
      }
    }

    system::~system() {
      lock_guard<mutex> guard(openvr_init_mutex);
      if (!--openvr_initializations) {
        VR_Shutdown();
        log("vr")->info("shutdown");
      } else {
        log("vr")->info("re-entrant shutdown {}", openvr_initializations + 1);
      }

    }

    string system::device_string(device_id index, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError * error) const {
      return detail::buffered_with_error(mem_fn(&vr::IVRSystem::GetStringTrackedDeviceProperty), error, handle, index, prop);
    }

    bool system::poll() const {
      // Process SteamVR events
      vr::VREvent_t event;
      while (handle->PollNextEvent(&event, sizeof(event))) {
        on_event(event);
        switch (event.eventType) {
          case VREvent_ModelSkinSettingsHaveChanged: on_model_skin_settings_have_changed(); break;
          case VREvent_IpdChanged: on_ipd_changed(); break; // so we can fiddle with the eye pose
          case VREvent_ProcessQuit: on_process_quit(); return true;
          case VREvent_Quit: on_quit(); return true; // TODO: permit blocking
          case VREvent_SceneApplicationSecondaryRenderingStarted: on_scene_application_secondary_rendering_started(event.data.process); break;
          case VREvent_InputFocusChanged: on_scene_application_changed(event.data.process); break;
          case VREvent_SceneApplicationChanged: on_scene_application_changed(event.data.process); break;
          case VREvent_SceneFocusChanged: on_scene_focus_changed(event.data.process); break;
          case VREvent_SceneFocusLost: on_scene_focus_lost(event.data.process); break;
          case VREvent_SceneFocusGained: on_scene_focus_gained(event.data.process); break;
          case VREvent_InputFocusCaptured: on_input_focus_captured(event.data.process); break;
          case VREvent_InputFocusReleased: on_input_focus_released(event.data.process); break;
          case VREvent_HideRenderModels: on_hide_render_models(); break;
          case VREvent_ShowRenderModels: on_show_render_models(); break;
          case VREvent_ButtonPress: on_button_press(event.data.controller); break;
          case VREvent_ButtonUnpress: on_button_unpress(event.data.controller); break;
          case VREvent_ButtonTouch: on_button_touch(event.data.controller); break;
          case VREvent_ButtonUntouch: on_button_untouch(event.data.controller); break;
          case VREvent_MouseButtonUp: on_mouse_button_up(event.data.mouse); break;
          case VREvent_MouseButtonDown: on_mouse_button_down(event.data.mouse); break;
          case VREvent_MouseMove: on_mouse_move(event.data.mouse); break;
          case VREvent_Scroll: on_scroll(event.data.mouse); break;
          case VREvent_TouchPadMove: on_touchpad_move(event.data.mouse); break;
          case VREvent_TrackedDeviceActivated: on_tracked_device_activated(); break;
          case VREvent_TrackedDeviceDeactivated: on_tracked_device_deactivated(); break;
          case VREvent_DashboardActivated: on_dashboard_activated(); break;
          case VREvent_DashboardDeactivated: on_dashboard_deactivated(); break;
          case VREvent_DashboardThumbSelected: on_dashboard_thumb_selected(event.data.overlay.overlayHandle); break;
          case VREvent_DashboardRequested: on_dashboard_requested(event.data.overlay.overlayHandle); break;
          case VREvent_Compositor_ChaperoneBoundsHidden: on_chaperone_bounds_hidden(); break;
          case VREvent_Compositor_ChaperoneBoundsShown: on_chaperone_bounds_shown(); break;
          case VREvent_ChaperoneTempDataHasChanged: on_chaperone_temp_data_has_changed(); break;
          case VREvent_ChaperoneDataHasChanged: on_chaperone_data_has_changed(); break;
          case VREvent_ChaperoneSettingsHaveChanged: on_chaperone_settings_have_changed(); break;
          case VREvent_ChaperoneUniverseHasChanged: on_chaperone_universe_changed(event.data.chaperone); break;
          default:
            log("vr")->info("unknown event type: {} ({})", show_event_type(event.eventType), event.eventType);
            break;
        }
      }

      // Process SteamVR controller state
      // for (int i = 0; i < device_id::max; i++) {
      //    system->GetControllerState(i, &controller_state[i]);
      // }
      return false;
    }



    // static signals
    signal<void(vr::VREvent_t &)> system::on_event;
    signal<void()> system::on_process_quit, system::on_quit, system::on_ipd_changed, system::on_hide_render_models, system::on_show_render_models, system::on_model_skin_settings_have_changed, system::on_dashboard_activated, system::on_dashboard_deactivated, system::on_chaperone_bounds_shown, system::on_chaperone_bounds_hidden, system::on_chaperone_data_has_changed, system::on_chaperone_settings_have_changed, system::on_chaperone_temp_data_has_changed, system::on_tracked_device_activated, system::on_tracked_device_deactivated;
    signal<void(process_id)> system::on_focus_enter, system::on_focus_leave, system::on_overlay_focus_changed, system::on_dashboard_thumb_selected, system::on_dashboard_requested;
    signal<void(vr::VREvent_Controller_t &)> system::on_button_press, system::on_button_unpress, system::on_button_touch, system::on_button_untouch;
    signal<void(vr::VREvent_Mouse_t &)> system::on_mouse_move, system::on_mouse_button_up, system::on_mouse_button_down, system::on_scroll, system::on_touchpad_move;
    signal<void(vr::VREvent_Chaperone_t &)> system::on_chaperone_universe_changed;
    signal<void(vr::VREvent_Process_t &)> system::on_scene_focus_lost, system::on_scene_focus_gained,
      system::on_scene_application_changed, system::on_scene_focus_changed, system::on_input_focus_changed, system::on_input_focus_captured, system::on_input_focus_released, system::on_scene_application_secondary_rendering_started;


    /*
    string openvr_system::component_name(string model, int i) {
      return buffered(mem_fn(&IVRRenderModels::GetComponentName), vr::VRRenderModels(), model.c_str(), i);
    }

    string openvr_system::component_model(string model, string cname) {
      return buffered(mem_fn(&IVRRenderModels::GetComponentRenderModelName), vr::VRRenderModels(), model.c_str(), cname.c_str());
    }

    string openvr_system::model_name(int i) {
      return buffered(mem_fn(&IVRRenderModels::GetRenderModelName), renderModels, i);
    }
  */

  }
}

#endif