#include "framework/stdafx.h"
#include "framework/std.h"
#include "framework/openvr_system.h"
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

    const char * system::show_tracked_device_class(TrackedDeviceClass c) const noexcept {
      switch (c) {
        case TrackedDeviceClass_Controller: return "controller";
        case TrackedDeviceClass_HMD: return "HMD";
        case TrackedDeviceClass_Invalid: return "invalid";
        case TrackedDeviceClass_Other: return "other";
        case TrackedDeviceClass_TrackingReference: return "tracking reference";
        default: return "unknown";
      }
    }

    const char * system::show_compositor_error(EVRCompositorError e) const noexcept {
      switch (e) {
        case VRCompositorError_None: return "none";
        case VRCompositorError_DoNotHaveFocus: return "do not have focus";
        case VRCompositorError_IncompatibleVersion: return "incompatible version";
        case VRCompositorError_IndexOutOfRange: return "index out of range";
        case VRCompositorError_InvalidTexture: return "invalid texture";
        case VRCompositorError_IsNotSceneApplication: return "is not scene application";
        case VRCompositorError_RequestFailed: return "request failed";
        case VRCompositorError_SharedTexturesNotSupported: return "shared textures not supported";
        case VRCompositorError_TextureIsOnWrongDevice: return "texture is on wrong device";
        case VRCompositorError_TextureUsesUnsupportedFormat: return "texture uses unsupported format";
        default: return "unknown";
      }
    }

    const char * system::show_event_type(int e) const noexcept {
      switch (e) {
        case VREvent_None: return "None";
        case VREvent_TrackedDeviceActivated: return "TrackedDeviceActivated";
        case VREvent_TrackedDeviceDeactivated: return "TrackedDeviceDeactivated";
        case VREvent_TrackedDeviceUpdated: return "TrackedDeviceUpdated";
        case VREvent_TrackedDeviceUserInteractionStarted: return "TrackedDeviceUserInteractionStarted";
        case VREvent_TrackedDeviceUserInteractionEnded: return "TrackedDeviceUserInteractionEnded";
        case VREvent_IpdChanged: return "IpdChanged";
        case VREvent_EnterStandbyMode: return "EnterStandbyMode";
        case VREvent_LeaveStandbyMode: return "LeaveStandbyMode";
        case VREvent_TrackedDeviceRoleChanged: return "TrackedDeviceRoleChanged";
        case VREvent_WatchdogWakeUpRequested: return "WatchdogWakeUpRequested";
        case VREvent_ButtonPress: return "ButtonPress"; // data is controller
        case VREvent_ButtonUnpress: return "ButtonUnpress"; // data is controller
        case VREvent_ButtonTouch: return "ButtonTouch"; // data is controller
        case VREvent_ButtonUntouch: return "ButtonUntouch"; // data is controller
        case VREvent_MouseMove: return "MouseMove"; // data is mouse
        case VREvent_MouseButtonDown: return "MouseButtonDown"; // data is mouse
        case VREvent_MouseButtonUp: return "MouseButtonUp"; // data is mouse
        case VREvent_FocusEnter: return "FocusEnter"; // data is overlay
        case VREvent_FocusLeave: return "FocusLeave"; // data is overlay
        case VREvent_Scroll: return "Scroll"; // data is mouse
        case VREvent_TouchPadMove: return "TouchPadMove"; // data is mouse
        case VREvent_OverlayFocusChanged: return "OverlayFocusChanged"; // data is overlay, global event
        case VREvent_InputFocusCaptured: return "InputFocusCaptured"; // data is process DEPRECATED
        case VREvent_InputFocusReleased: return "InputFocusReleased"; // data is process DEPRECATED
        case VREvent_SceneFocusLost: return "SceneFocusLost"; // data is process
        case VREvent_SceneFocusGained: return "SceneFocusGained"; // data is process
        case VREvent_SceneApplicationChanged: return "SceneApplicationChanged"; // data is process - The App actually drawing the scene changed (usually to or from the compositor)
        case VREvent_SceneFocusChanged: return "SceneFocusChanged"; // data is process - New app got access to draw the scene
        case VREvent_InputFocusChanged: return "InputFocusChanged"; // data is process
        case VREvent_SceneApplicationSecondaryRenderingStarted: return "SceneApplicationSecondaryRenderingStarted"; // data is process
        case VREvent_HideRenderModels: return "HideRenderModels"; // Sent to the scene application to request hiding render models temporarily
        case VREvent_ShowRenderModels: return "ShowRenderModels"; // Sent to the scene application to request restoring render model visibility
        case VREvent_OverlayShown: return "OverlayShown";
        case VREvent_OverlayHidden: return "OverlayHidden";
        case VREvent_DashboardActivated: return "DashboardActivated";
        case VREvent_DashboardDeactivated: return "DashboardDeactivated";
        case VREvent_DashboardThumbSelected: return "DashboardThumbSelected"; // Sent to the overlay manager - data is overlay
        case VREvent_DashboardRequested: return "DashboardRequested"; // Sent to the overlay manager - data is overlay
        case VREvent_ResetDashboard: return "ResetDashboard"; // Send to the overlay manager
        case VREvent_RenderToast: return "RenderToast"; // Send to the dashboard to render a toast - data is the notification ID
        case VREvent_ImageLoaded: return "ImageLoaded"; // Sent to overlays when a SetOverlayRaw or SetOverlayFromFile call finishes loading
        case VREvent_ShowKeyboard: return "ShowKeyboard"; // Sent to keyboard renderer in the dashboard to invoke it
        case VREvent_HideKeyboard: return "HideKeyboard"; // Sent to keyboard renderer in the dashboard to hide it
        case VREvent_OverlayGamepadFocusGained: return "OverlayGamepadFocusGained"; // Sent to an overlay when IVROverlay::VREvent_SetFocusOverlay is called on it
        case VREvent_OverlayGamepadFocusLost: return "OverlayGamepadFocusLost"; // Send to an overlay when it previously had focus and IVROverlay::VREvent_SetFocusOverlay is called on something else
        case VREvent_OverlaySharedTextureChanged: return "OverlaySharedTextureChanged";
        case VREvent_DashboardGuideButtonDown: return "DashboardGuideButtonDown";
        case VREvent_DashboardGuideButtonUp: return "DashboardGuideButtonUp";
        case VREvent_ScreenshotTriggered: return "ScreenshotTriggered"; // Screenshot button combo was pressed, Dashboard should request a screenshot
        case VREvent_ImageFailed: return "ImageFailed"; // Sent to overlays when a SetOverlayRaw or SetOverlayfromFail fails to load
                                                                      // Screenshot API
        case VREvent_RequestScreenshot: return "RequestScreenshot"; // Sent by vrclient application to compositor to take a screenshot
        case VREvent_ScreenshotTaken: return "ScreenshotTaken"; // Sent by compositor to the application that the screenshot has been taken
        case VREvent_ScreenshotFailed: return "ScreenshotFailed"; // Sent by compositor to the application that the screenshot failed to be taken
        case VREvent_SubmitScreenshotToDashboard: return "SubmitScreenshotToDashboard"; // Sent by compositor to the dashboard that a completed screenshot was submitted
        case VREvent_ScreenshotProgressToDashboard: return "ScreenshotProgressToDashboard"; // Sent by compositor to the dashboard that a completed screenshot was submitted
        case VREvent_Notification_Shown: return "Notification_Shown";
        case VREvent_Notification_Hidden: return "Notification_Hidden";
        case VREvent_Notification_BeginInteraction: return "Notification_BeginInteraction";
        case VREvent_Notification_Destroyed: return "Notification_Destroyed";
        case VREvent_Quit: return "Quit"; // data is process
        case VREvent_ProcessQuit: return "ProcessQuit"; // data is process
        case VREvent_QuitAborted_UserPrompt: return "QuitAborted_UserPrompt"; // data is process
        case VREvent_QuitAcknowledged: return "QuitAcknowledged"; // data is process
        case VREvent_DriverRequestedQuit: return "DriverRequestedQuit"; // The driver has requested that SteamVR shut down
        case VREvent_ChaperoneDataHasChanged: return "ChaperoneDataHasChanged";
        case VREvent_ChaperoneUniverseHasChanged: return "ChaperoneUniverseHasChanged";
        case VREvent_ChaperoneTempDataHasChanged: return "ChaperoneTempDataHasChanged";
        case VREvent_ChaperoneSettingsHaveChanged: return "ChaperoneSettingsHaveChanged";
        case VREvent_SeatedZeroPoseReset: return "SeatedZeroPoseReset";
        case VREvent_AudioSettingsHaveChanged: return "AudioSettingsHaveChanged";
        case VREvent_BackgroundSettingHasChanged: return "BackgroundSettingHasChanged";
        case VREvent_CameraSettingsHaveChanged: return "CameraSettingsHaveChanged";
        case VREvent_ReprojectionSettingHasChanged: return "ReprojectionSettingHasChanged";
        case VREvent_ModelSkinSettingsHaveChanged: return "ModelSkinSettingsHaveChanged";
        case VREvent_EnvironmentSettingsHaveChanged: return "EnvironmentSettingsHaveChanged";
        case VREvent_StatusUpdate: return "StatusUpdate";
        case VREvent_MCImageUpdated: return "MCImageUpdated";
        case VREvent_FirmwareUpdateStarted: return "FirmwareUpdateStarted";
        case VREvent_FirmwareUpdateFinished: return "FirmwareUpdateFinished";
        case VREvent_KeyboardClosed: return "KeyboardClosed";
        case VREvent_KeyboardCharInput: return "KeyboardCharInput";
        case VREvent_KeyboardDone: return "KeyboardDone"; // Sent when DONE button clicked on keyboard
        case VREvent_ApplicationTransitionStarted: return "ApplicationTransitionStarted";
        case VREvent_ApplicationTransitionAborted: return "ApplicationTransitionAborted";
        case VREvent_ApplicationTransitionNewAppStarted: return "ApplicationTransitionNewAppStarted";
        case VREvent_ApplicationListUpdated: return "ApplicationListUpdated";
        case VREvent_ApplicationMimeTypeLoad: return "ApplicationMimeTypeLoad";
        case VREvent_Compositor_MirrorWindowShown: return "Compositor_MirrorWindowShown";
        case VREvent_Compositor_MirrorWindowHidden: return "Compositor_MirrorWindowHidden";
        case VREvent_Compositor_ChaperoneBoundsShown: return "Compositor_ChaperoneBoundsShown";
        case VREvent_Compositor_ChaperoneBoundsHidden: return "Compositor_ChaperoneBoundsHidden";
        case VREvent_TrackedCamera_StartVideoStream: return "TrackedCamera_StartVideoStream";
        case VREvent_TrackedCamera_StopVideoStream: return "TrackedCamera_StopVideoStream";
        case VREvent_TrackedCamera_PauseVideoStream: return "TrackedCamera_PauseVideoStream";
        case VREvent_TrackedCamera_ResumeVideoStream: return "TrackedCamera_ResumeVideoStream";
        case VREvent_PerformanceTest_EnableCapture: return "PerformanceTest_EnableCapture";
        case VREvent_PerformanceTest_DisableCapture: return "PerformanceTest_DisableCapture";
        case VREvent_PerformanceTest_FidelityLevel: return "PerformanceTest_FidelityLevel";
        default: return "unknown";
      }
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
