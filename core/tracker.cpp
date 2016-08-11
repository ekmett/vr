#include <set>
#include <string>
#include <stdio.h>
#include "tracker.h"
#include "util.h"

using namespace std;
using namespace vr;
using namespace spdlog;

namespace core {

  level::level_enum chaperone_calibration_log_level(ChaperoneCalibrationState state) {
    if (state >= ChaperoneCalibrationState::ChaperoneCalibrationState_Error) return spdlog::level::err;
    if (state >= ChaperoneCalibrationState::ChaperoneCalibrationState_Warning) return spdlog::level::warn;
    return level::info;
  }

  openvr_tracker::openvr_tracker(shared_ptr<logger> & log)
    : log(log)
    , focus_lost(false)
    , dashboard_active(false)
    , chaperone_bounds_visible(true) {
    if (!VR_IsHmdPresent())
      die("No head mounted device detected.\n");

    // Initialize OpenVR
    auto eError = VRInitError_None;
    hmd = VR_Init(&eError, VRApplication_Scene);

    if (eError != VRInitError_None)
      die("Unable to initialize OpenVR.\n%s", VR_GetVRInitErrorAsEnglishDescription(eError));

    try {

      log->info("driver: {}", driver());
      log->info("serial #: {}", serial_number());

      renderModels = (IVRRenderModels *)VR_GetGenericInterface(IVRRenderModels_Version, &eError);
      if (!renderModels)
        die("Unable to get render model interface.\n%s", VR_GetVRInitErrorAsEnglishDescription(eError));

      /*
      int modelCount = renderModels->GetRenderModelCount();

      log->info("render models: {}", modelCount);

      for (int i = 0;i < modelCount;++i) {
        string name = render_model_name(i);
        log->info("render model {}: {}", i, name, components);
      }
      */

      // scan to determine the live model names
      set<string> model_names;

      for (int i = 0;i < vr::k_unMaxTrackedDeviceCount; ++i) {
        auto device_class = hmd->GetTrackedDeviceClass(i);
        if (device_class == vr::TrackedDeviceClass_Invalid) continue;

        TrackedPropertyError err = vr::TrackedProp_Success;
        string model_name = device_string(i, Prop_RenderModelName_String, &err);
        if (err == TrackedProp_Success) model_names.insert(model_name);

        log->info("device {}: class: {} ({}), model: {}", i, show_tracked_device_class(device_class), device_class, model_name);

        switch (device_class) {
        case TrackedDeviceClass_Controller:
          //hmd->GetStringTrackedDeviceProperty(i,TrackedProp)
        default: break;
        }
      }

      for (auto name : model_names) {
        int components = renderModels->GetComponentCount(name.c_str());
        log->info("model {}: {} components", name, components);
        for (int i = 0;i < components;i++) {
          string cname = component_name(name, i);
          uint64_t button_mask = renderModels->GetComponentButtonMask(name.c_str(), cname.c_str());
          VRControllerState_t controller_state;
          RenderModel_ControllerMode_State_t controllermode_state;
          RenderModel_ComponentState_t component_state;
          renderModels->GetComponentState(name.c_str(), cname.c_str(), &controller_state, &controllermode_state, &component_state);
          log->info("component {}: {}, {} {} {} {} {}, buttons: {}", i, cname,
            (component_state.uProperties & VRComponentProperty_IsStatic) ? "static" : "dynamic",
            (component_state.uProperties & VRComponentProperty_IsVisible) ? "visible" : "invisible",
            (component_state.uProperties & VRComponentProperty_IsPressed) ? "pressed" : "unpressed",
            (component_state.uProperties & VRComponentProperty_IsScrolled) ? "scrolled" : "unscrolled",
            (component_state.uProperties & VRComponentProperty_IsTouched) ? "touched" : "untouched",
            button_mask);
        }
      }

      compositor = vr::VRCompositor();
      if (!compositor)
        die("Unable to initialize OpenVR compositor.\n");

      log->info("compositor: can render scene? {}", compositor->CanRenderScene());
      log->info("compositor: should render with low resources? {}", compositor->ShouldAppRenderWithLowResources());


      chaperone = vr::VRChaperone();

      // TODO: modify this to be cool with losing the chaperone, fade to grid whenever we aren't "ready"
      if (!chaperone)
        die("unable to retrieve OpenVR chaperone.\n");

      recalculate_chaperone_data();

    } catch (...) {
      VR_Shutdown();
      throw;
    }
  }

  template<typename... Ts, typename f> string buffered(f fun, Ts... args) {
    uint32_t len = fun(args..., (char *) nullptr, uint32_t(0));
    if (len == 0) return "";
    char *buffer = new char[len];
    len = fun(args..., buffer, len);
    string result = buffer;
    delete[] buffer;
    return result;
  }

  template<typename... Ts, typename f, typename err> string buffered_with_error(f fun, err *e, Ts... args) {
    uint32_t len = fun(args..., nullptr, 0, e);
    if (len == 0) return "";
    char *buffer = new char[len];
    len = fun(args..., buffer, len, e);
    string result = buffer;
    delete[] buffer;
    return result;
  }

  string openvr_tracker::component_name(string model, int i) {
    return buffered(mem_fn(&IVRRenderModels::GetComponentName), renderModels, model.c_str(), i);
  }

  string openvr_tracker::component_model(string model, string cname) {
    return buffered(mem_fn(&IVRRenderModels::GetComponentRenderModelName), renderModels, model.c_str(), cname.c_str());
  }

  string openvr_tracker::device_string(TrackedDeviceIndex_t device, TrackedDeviceProperty prop, TrackedPropertyError *error) {
    return buffered_with_error(mem_fn(&IVRSystem::GetStringTrackedDeviceProperty), error, hmd, device, prop);
  }

  
  string openvr_tracker::model_name(int i) {
    return buffered(mem_fn(&IVRRenderModels::GetRenderModelName), renderModels, i);
  }

  string openvr_tracker::driver() {
    return device_string(k_unTrackedDeviceIndex_Hmd, Prop_TrackingSystemName_String);
  }

  string openvr_tracker::serial_number() {
    return device_string(k_unTrackedDeviceIndex_Hmd, Prop_SerialNumber_String);
  }

  void openvr_tracker::recalculate_chaperone_data() {
    auto calibrationState = chaperone->GetCalibrationState();

    log->log(chaperone_calibration_log_level(calibrationState), "chaperone calibration state: {} ({})", show_chaperone_calibration_state(calibrationState), (int)calibrationState);

    float playX = 0.f, playZ = 0.f;
    vr::HmdQuad_t playArea[4];

    bool chaperoned = chaperone->GetPlayAreaSize(&playX, &playZ)
      && chaperone->GetPlayAreaRect(playArea);

    if (chaperoned) {
      log->info("chaperoned play area {:.2f}x{:.2f}", playX, playZ);
    } else {
      log->warn("unchaperoned");

    }
  }

  openvr_tracker::~openvr_tracker() {
    VR_Shutdown();
  }

  bool openvr_tracker::poll() {
    // Process SteamVR events
    vr::VREvent_t event;
    while (hmd->PollNextEvent(&event, sizeof(event))) {
      bool handling = true;
      switch (event.eventType) {

        case VREvent_ModelSkinSettingsHaveChanged: on_model_skin_settings_have_changed(); break;
        case VREvent_IpdChanged: on_ipd_changed(); break; // so we can fiddle with the eye pose


        case VREvent_ProcessQuit:
        case VREvent_Quit: on_quit(); return true;

        // process management
        case VREvent_SceneApplicationSecondaryRenderingStarted: on_scene_application_secondary_rendering_started(event.data.process); break;
        case VREvent_InputFocusChanged: on_scene_application_changed(event.data.process); break;
        case VREvent_SceneApplicationChanged: on_scene_application_changed(event.data.process); break;
        case VREvent_SceneFocusChanged: on_scene_focus_changed(event.data.process); break;
        case VREvent_SceneFocusLost: on_scene_focus_lost(event.data.process); break;
        case VREvent_SceneFocusGained: on_scene_focus_gained(event.data.process); break;
        case VREvent_InputFocusCaptured: focus_lost = true; on_input_focus_captured(event.data.process); break;
        case VREvent_InputFocusReleased: focus_lost = false; on_input_focus_released(event.data.process); break;

        // models
        case VREvent_HideRenderModels: on_hide_render_models(); break;
        case VREvent_ShowRenderModels: on_show_render_models(); break;

        // mouse
        case VREvent_ButtonPress: on_button_press(event.data.controller); break;
        case VREvent_ButtonUnpress: on_button_unpress(event.data.controller); break;
        case VREvent_ButtonTouch: on_button_touch(event.data.controller); break;
        case VREvent_ButtonUntouch: on_button_untouch(event.data.controller); break;
        case VREvent_MouseButtonUp: on_mouse_button_up(event.data.mouse); break;
        case VREvent_MouseButtonDown: on_mouse_button_down(event.data.mouse); break;
        case VREvent_MouseMove: on_mouse_move(event.data.mouse); break;
        case VREvent_Scroll: on_scroll(event.data.mouse); break;
        case VREvent_TouchPadMove: on_touchpad_move(event.data.mouse); break;


        // dashboard activity
        case VREvent_DashboardActivated: dashboard_active = true; on_dashboard_activated(); break;
        case VREvent_DashboardDeactivated: dashboard_active = false; on_dashboard_deactivated(); break;
        case VREvent_DashboardThumbSelected: on_dashboard_thumb_selected(event.data.overlay.overlayHandle); break;
        case VREvent_DashboardRequested: on_dashboard_requested(event.data.overlay.overlayHandle); break;

        // chaperone data
        case VREvent_Compositor_ChaperoneBoundsHidden: chaperone_bounds_visible = false; on_chaperone_bounds_hidden(); break;
        case VREvent_Compositor_ChaperoneBoundsShown: chaperone_bounds_visible = true; on_chaperone_bounds_shown(); break;
        case VREvent_ChaperoneTempDataHasChanged: on_chaperone_temp_data_has_changed(); break;
        case VREvent_ChaperoneDataHasChanged: on_chaperone_data_has_changed(); break;
        case VREvent_ChaperoneSettingsHaveChanged: on_chaperone_settings_have_changed(); break;
        case VREvent_ChaperoneUniverseHasChanged:
          // we receive one of these on app start
          log->info("changed chaperone universe from {} to {}", event.data.chaperone.m_nPreviousUniverse, event.data.chaperone.m_nCurrentUniverse);
          recalculate_chaperone_data();
          on_chaperone_universe_changed(event.data.chaperone);
          break;

        default:
          handling = false;
          break;
      } 
      if (!handling) log->info("unhandled event: {}", show_event_type((EVREventType)event.eventType));

    }
    return false;
  }

}