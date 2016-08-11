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
    // return buffered([&](auto x, auto y) { return renderModels->GetComponentName(model.c_str(), i, x, y); });
    return buffered(mem_fn(&IVRRenderModels::GetComponentName), renderModels, model.c_str(), i);
  }

  string openvr_tracker::device_string(TrackedDeviceIndex_t device, TrackedDeviceProperty prop, TrackedPropertyError *error) {
    return buffered_with_error(mem_fn(&IVRSystem::GetStringTrackedDeviceProperty), error, hmd, device, prop);
  }

  string openvr_tracker::model_name(int i) {
    return buffered(mem_fn(&IVRRenderModels::GetRenderModelName), renderModels, i);
    /*
    uint32_t len = renderModels->GetRenderModelName(i, nullptr, 0);
    if (len == 0) return "";
    char *buffer = new char[len];
    len = renderModels->GetRenderModelName(i, buffer, len);
    string result = buffer;
    delete[] buffer;
    return result;
    */
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
      case EVREventType::VREvent_ProcessQuit:
      case EVREventType::VREvent_Quit:
        on_quit();
        return true;

      case EVREventType::VREvent_IpdChanged: on_ipd_changed(); break;
      case EVREventType::VREvent_InputFocusChanged: break;
      case EVREventType::VREvent_InputFocusCaptured: focus_lost = true; on_focus_captured(); break;
      case EVREventType::VREvent_InputFocusReleased: focus_lost = false; on_focus_released(); break;

      case EVREventType::VREvent_DashboardActivated: dashboard_active = true; on_dashboard_activated(); break;
      case EVREventType::VREvent_DashboardDeactivated: dashboard_active = false; on_dashboard_deactivated(); break;
      case EVREventType::VREvent_Compositor_ChaperoneBoundsHidden: chaperone_bounds_visible = false; on_chaperone_bounds_hidden(); break;
      case EVREventType::VREvent_Compositor_ChaperoneBoundsShown: chaperone_bounds_visible = true; on_chaperone_bounds_shown(); break;

      case EVREventType::VREvent_ChaperoneUniverseHasChanged:
        // we receive one of these on app start
        log->info("changed chaperone universe from {} to {}", event.data.chaperone.m_nPreviousUniverse, event.data.chaperone.m_nCurrentUniverse);
        recalculate_chaperone_data();
        on_chaperone_universe_changed(event.data.chaperone.m_nPreviousUniverse, event.data.chaperone.m_nCurrentUniverse);
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