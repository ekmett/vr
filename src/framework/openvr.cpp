#include "framework/stdafx.h"
#include "framework/openvr.h"

namespace framework {
  namespace openvr {

    const char * show_tracked_device_class(TrackedDeviceClass c) noexcept {
      switch (c) {
        case TrackedDeviceClass_Controller: return "controller";
        case TrackedDeviceClass_HMD: return "HMD";
        case TrackedDeviceClass_Invalid: return "invalid";
        case TrackedDeviceClass_Other: return "other";
        case TrackedDeviceClass_TrackingReference: return "tracking reference";
        default: return "unknown";
      }
    }

    const char * show_compositor_error(EVRCompositorError e) noexcept {
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

    const char * show_event_type(int e) noexcept {
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
  }
}