#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <stdexcept>
#include <openvr.h>
#include <windows.h>
#include <debugapi.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util.h"
#include "log.h"

using namespace vr;

namespace core {

  const char * show_object_label_type(GLenum t) {
    switch (t) {
      case GL_BUFFER: return "buffer";
      case GL_SHADER: return "shader";
      case GL_PROGRAM: return "program";
      case GL_VERTEX_ARRAY: return "vao";
      case GL_QUERY: return "query";
      case GL_PROGRAM_PIPELINE: return "program pipeline";
      case GL_TRANSFORM_FEEDBACK: return "transform feedback";
      case GL_SAMPLER: return "sampler";
      case GL_TEXTURE: return "texture";
      case GL_RENDERBUFFER: return "renderbuffer";
      case GL_FRAMEBUFFER: return "framebuffer";
      default: return "unknown";
    }
  }

  const char * show_debug_source(GLenum s) {
    switch (s) {
    case GL_DEBUG_SOURCE_API: return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "window system";
    case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY: return "third party";
    case GL_DEBUG_SOURCE_APPLICATION: return "application";
    case GL_DEBUG_SOURCE_OTHER: return "other";
    default: return "unknown";
    }
  }

  const char * show_debug_message_type(GLenum t) {
    switch (t) {
    case GL_DEBUG_TYPE_ERROR: return "error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated behavior";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "undefined behavior";
    case GL_DEBUG_TYPE_PORTABILITY: return "portability";
    case GL_DEBUG_TYPE_PERFORMANCE: return "performance";
    case GL_DEBUG_TYPE_MARKER: return "marker";
    case GL_DEBUG_TYPE_PUSH_GROUP: return "push group";
    case GL_DEBUG_TYPE_POP_GROUP: return "pop group";
    case GL_DEBUG_TYPE_OTHER: return "other";
    default: return "unknown";
    }
  }

  const char * show_chaperone_calibration_state(ChaperoneCalibrationState state) {
    switch (state) {
    case ChaperoneCalibrationState::ChaperoneCalibrationState_OK: return "ok";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Warning: return "warning";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Warning_BaseStationMayHaveMoved: return "base station may have moved";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Warning_BaseStationRemoved: return "base station removed";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Warning_SeatedBoundsInvalid: return "seated bounds invalid";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Error: return "error";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Error_BaseStationUninitalized: return "base station uninitialized";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Error_BaseStationConflict: return "base station conflict";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Error_PlayAreaInvalid: return "play area invalid";
    case ChaperoneCalibrationState::ChaperoneCalibrationState_Error_CollisionBoundsInvalid: return "collision bounds invalid";
    default: return "unknown";
    }
  }

  const char * show_tracked_device_class(TrackedDeviceClass c) {
    switch (c) {
    case TrackedDeviceClass_Controller: return "controller";
    case TrackedDeviceClass_HMD: return "HMD";
    case TrackedDeviceClass_Invalid: return "invalid";
    case TrackedDeviceClass_Other: return "other";
    case TrackedDeviceClass_TrackingReference: return "tracking reference";
    default: return "unknown";
    }
  }

  const char * show_event_type(EVREventType e) {
    switch (e) {
    case EVREventType::VREvent_None: return "None";
    case EVREventType::VREvent_TrackedDeviceActivated: return "TrackedDeviceActivated";
    case EVREventType::VREvent_TrackedDeviceDeactivated: return "TrackedDeviceDeactivated";
    case EVREventType::VREvent_TrackedDeviceUpdated: return "TrackedDeviceUpdated";
    case EVREventType::VREvent_TrackedDeviceUserInteractionStarted: return "TrackedDeviceUserInteractionStarted";
    case EVREventType::VREvent_TrackedDeviceUserInteractionEnded: return "TrackedDeviceUserInteractionEnded";
    case EVREventType::VREvent_IpdChanged: return "IpdChanged";
    case EVREventType::VREvent_EnterStandbyMode: return "EnterStandbyMode";
    case EVREventType::VREvent_LeaveStandbyMode: return "LeaveStandbyMode";
    case EVREventType::VREvent_TrackedDeviceRoleChanged: return "TrackedDeviceRoleChanged";
    case EVREventType::VREvent_WatchdogWakeUpRequested: return "WatchdogWakeUpRequested";
    case EVREventType::VREvent_ButtonPress: return "ButtonPress"; // data is controller
    case EVREventType::VREvent_ButtonUnpress: return "ButtonUnpress"; // data is controller
    case EVREventType::VREvent_ButtonTouch: return "ButtonTouch"; // data is controller
    case EVREventType::VREvent_ButtonUntouch: return "ButtonUntouch"; // data is controller
    case EVREventType::VREvent_MouseMove: return "MouseMove"; // data is mouse
    case EVREventType::VREvent_MouseButtonDown: return "MouseButtonDown"; // data is mouse
    case EVREventType::VREvent_MouseButtonUp: return "MouseButtonUp"; // data is mouse
    case EVREventType::VREvent_FocusEnter: return "FocusEnter"; // data is overlay
    case EVREventType::VREvent_FocusLeave: return "FocusLeave"; // data is overlay
    case EVREventType::VREvent_Scroll: return "Scroll"; // data is mouse
    case EVREventType::VREvent_TouchPadMove: return "TouchPadMove"; // data is mouse
    case EVREventType::VREvent_OverlayFocusChanged: return "OverlayFocusChanged"; // data is overlay, global event
    case EVREventType::VREvent_InputFocusCaptured: return "InputFocusCaptured"; // data is process DEPRECATED
    case EVREventType::VREvent_InputFocusReleased: return "InputFocusReleased"; // data is process DEPRECATED
    case EVREventType::VREvent_SceneFocusLost: return "SceneFocusLost"; // data is process
    case EVREventType::VREvent_SceneFocusGained: return "SceneFocusGained"; // data is process
    case EVREventType::VREvent_SceneApplicationChanged: return "SceneApplicationChanged"; // data is process - The App actually drawing the scene changed (usually to or from the compositor)
    case EVREventType::VREvent_SceneFocusChanged: return "SceneFocusChanged"; // data is process - New app got access to draw the scene
    case EVREventType::VREvent_InputFocusChanged: return "InputFocusChanged"; // data is process
    case EVREventType::VREvent_SceneApplicationSecondaryRenderingStarted: return "SceneApplicationSecondaryRenderingStarted"; // data is process
    case EVREventType::VREvent_HideRenderModels: return "HideRenderModels"; // Sent to the scene application to request hiding render models temporarily
    case EVREventType::VREvent_ShowRenderModels: return "ShowRenderModels"; // Sent to the scene application to request restoring render model visibility
    case EVREventType::VREvent_OverlayShown: return "OverlayShown";
    case EVREventType::VREvent_OverlayHidden: return "OverlayHidden";
    case EVREventType::VREvent_DashboardActivated: return "DashboardActivated";
    case EVREventType::VREvent_DashboardDeactivated: return "DashboardDeactivated";
    case EVREventType::VREvent_DashboardThumbSelected: return "DashboardThumbSelected"; // Sent to the overlay manager - data is overlay
    case EVREventType::VREvent_DashboardRequested: return "DashboardRequested"; // Sent to the overlay manager - data is overlay
    case EVREventType::VREvent_ResetDashboard: return "ResetDashboard"; // Send to the overlay manager
    case EVREventType::VREvent_RenderToast: return "RenderToast"; // Send to the dashboard to render a toast - data is the notification ID
    case EVREventType::VREvent_ImageLoaded: return "ImageLoaded"; // Sent to overlays when a SetOverlayRaw or SetOverlayFromFile call finishes loading
    case EVREventType::VREvent_ShowKeyboard: return "ShowKeyboard"; // Sent to keyboard renderer in the dashboard to invoke it
    case EVREventType::VREvent_HideKeyboard: return "HideKeyboard"; // Sent to keyboard renderer in the dashboard to hide it
    case EVREventType::VREvent_OverlayGamepadFocusGained: return "OverlayGamepadFocusGained"; // Sent to an overlay when IVROverlay::VREvent_SetFocusOverlay is called on it
    case EVREventType::VREvent_OverlayGamepadFocusLost: return "OverlayGamepadFocusLost"; // Send to an overlay when it previously had focus and IVROverlay::VREvent_SetFocusOverlay is called on something else
    case EVREventType::VREvent_OverlaySharedTextureChanged: return "OverlaySharedTextureChanged";
    case EVREventType::VREvent_DashboardGuideButtonDown: return "DashboardGuideButtonDown";
    case EVREventType::VREvent_DashboardGuideButtonUp: return "DashboardGuideButtonUp";
    case EVREventType::VREvent_ScreenshotTriggered: return "ScreenshotTriggered"; // Screenshot button combo was pressed, Dashboard should request a screenshot
    case EVREventType::VREvent_ImageFailed: return "ImageFailed"; // Sent to overlays when a SetOverlayRaw or SetOverlayfromFail fails to load
                                                                  // Screenshot API
    case EVREventType::VREvent_RequestScreenshot: return "RequestScreenshot"; // Sent by vrclient application to compositor to take a screenshot
    case EVREventType::VREvent_ScreenshotTaken: return "ScreenshotTaken"; // Sent by compositor to the application that the screenshot has been taken
    case EVREventType::VREvent_ScreenshotFailed: return "ScreenshotFailed"; // Sent by compositor to the application that the screenshot failed to be taken
    case EVREventType::VREvent_SubmitScreenshotToDashboard: return "SubmitScreenshotToDashboard"; // Sent by compositor to the dashboard that a completed screenshot was submitted
    case EVREventType::VREvent_ScreenshotProgressToDashboard: return "ScreenshotProgressToDashboard"; // Sent by compositor to the dashboard that a completed screenshot was submitted
    case EVREventType::VREvent_Notification_Shown: return "Notification_Shown";
    case EVREventType::VREvent_Notification_Hidden: return "Notification_Hidden";
    case EVREventType::VREvent_Notification_BeginInteraction: return "Notification_BeginInteraction";
    case EVREventType::VREvent_Notification_Destroyed: return "Notification_Destroyed";
    case EVREventType::VREvent_Quit: return "Quit"; // data is process
    case EVREventType::VREvent_ProcessQuit: return "ProcessQuit"; // data is process
    case EVREventType::VREvent_QuitAborted_UserPrompt: return "QuitAborted_UserPrompt"; // data is process
    case EVREventType::VREvent_QuitAcknowledged: return "QuitAcknowledged"; // data is process
    case EVREventType::VREvent_DriverRequestedQuit: return "DriverRequestedQuit"; // The driver has requested that SteamVR shut down
    case EVREventType::VREvent_ChaperoneDataHasChanged: return "ChaperoneDataHasChanged";
    case EVREventType::VREvent_ChaperoneUniverseHasChanged: return "ChaperoneUniverseHasChanged";
    case EVREventType::VREvent_ChaperoneTempDataHasChanged: return "ChaperoneTempDataHasChanged";
    case EVREventType::VREvent_ChaperoneSettingsHaveChanged: return "ChaperoneSettingsHaveChanged";
    case EVREventType::VREvent_SeatedZeroPoseReset: return "SeatedZeroPoseReset";
    case EVREventType::VREvent_AudioSettingsHaveChanged: return "AudioSettingsHaveChanged";
    case EVREventType::VREvent_BackgroundSettingHasChanged: return "BackgroundSettingHasChanged";
    case EVREventType::VREvent_CameraSettingsHaveChanged: return "CameraSettingsHaveChanged";
    case EVREventType::VREvent_ReprojectionSettingHasChanged: return "ReprojectionSettingHasChanged";
    case EVREventType::VREvent_ModelSkinSettingsHaveChanged: return "ModelSkinSettingsHaveChanged";
    case EVREventType::VREvent_EnvironmentSettingsHaveChanged: return "EnvironmentSettingsHaveChanged";
    case EVREventType::VREvent_StatusUpdate: return "StatusUpdate";
    case EVREventType::VREvent_MCImageUpdated: return "MCImageUpdated";
    case EVREventType::VREvent_FirmwareUpdateStarted: return "FirmwareUpdateStarted";
    case EVREventType::VREvent_FirmwareUpdateFinished: return "FirmwareUpdateFinished";
    case EVREventType::VREvent_KeyboardClosed: return "KeyboardClosed";
    case EVREventType::VREvent_KeyboardCharInput: return "KeyboardCharInput";
    case EVREventType::VREvent_KeyboardDone: return "KeyboardDone"; // Sent when DONE button clicked on keyboard
    case EVREventType::VREvent_ApplicationTransitionStarted: return "ApplicationTransitionStarted";
    case EVREventType::VREvent_ApplicationTransitionAborted: return "ApplicationTransitionAborted";
    case EVREventType::VREvent_ApplicationTransitionNewAppStarted: return "ApplicationTransitionNewAppStarted";
    case EVREventType::VREvent_ApplicationListUpdated: return "ApplicationListUpdated";
    case EVREventType::VREvent_ApplicationMimeTypeLoad: return "ApplicationMimeTypeLoad";
    case EVREventType::VREvent_Compositor_MirrorWindowShown: return "Compositor_MirrorWindowShown";
    case EVREventType::VREvent_Compositor_MirrorWindowHidden: return "Compositor_MirrorWindowHidden";
    case EVREventType::VREvent_Compositor_ChaperoneBoundsShown: return "Compositor_ChaperoneBoundsShown";
    case EVREventType::VREvent_Compositor_ChaperoneBoundsHidden: return "Compositor_ChaperoneBoundsHidden";
    case EVREventType::VREvent_TrackedCamera_StartVideoStream: return "TrackedCamera_StartVideoStream";
    case EVREventType::VREvent_TrackedCamera_StopVideoStream: return "TrackedCamera_StopVideoStream";
    case EVREventType::VREvent_TrackedCamera_PauseVideoStream: return "TrackedCamera_PauseVideoStream";
    case EVREventType::VREvent_TrackedCamera_ResumeVideoStream: return "TrackedCamera_ResumeVideoStream";
    case EVREventType::VREvent_PerformanceTest_EnableCapture: return "PerformanceTest_EnableCapture";
    case EVREventType::VREvent_PerformanceTest_DisableCapture: return "PerformanceTest_DisableCapture";
    case EVREventType::VREvent_PerformanceTest_FidelityLevel: return "PerformanceTest_FidelityLevel";
    default: return "unknown";
    }
  }


  void objectLabelf(GLenum id, GLuint name, const char *fmt, ...) {
    va_list args;
    char buffer[2048];

    va_start(args, fmt);
    vsprintf_s(buffer, fmt, args);
    va_end(args);

    buffer[2047] = 0;
    glObjectLabel(id, name, static_cast<GLsizei>(strnlen_s(buffer, 2048)), buffer);
    auto gl_log = spdlog::get("gl");
    if (gl_log) {
      gl_log->info("labeled {} #{}: {}", show_object_label_type(id), name, buffer);
    }
  }

  void die_helper(const char * title, const char *fmt, ...) {
    va_list args;
    char buffer[2048];

    va_start(args, fmt);
    vsprintf_s(buffer, fmt, args);
    va_end(args);
    buffer[2047] = 0;
    OutputDebugStringA(buffer);
    throw std::runtime_error(buffer);
  }

}