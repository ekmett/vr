#include <string>
#include <stdio.h>
#include "tracker.h"
#include "util.h"

using namespace std;
using namespace vr;

string get_tracked_device_string(IVRSystem *hmd, TrackedDeviceIndex_t device, TrackedDeviceProperty prop, TrackedPropertyError *error = NULL) {
  uint32_t len = hmd->GetStringTrackedDeviceProperty(device, prop, NULL, 0, error);
  if (len == 0) return "";
  char *buffer = new char[len];
  len = hmd->GetStringTrackedDeviceProperty(device, prop, buffer, len, error);
  string result = buffer;
  delete[] buffer;
  return result;
}

openvr_tracker::openvr_tracker() {
  if (!VR_IsHmdPresent()) {
    die("No head mounted device detected.\n");
  }

  // Initialize OpenVR
  auto eError = VRInitError_None;
  hmd = VR_Init(&eError, VRApplication_Scene);

  if (eError != VRInitError_None)
    die("Unable to initialize OpenVR.\n%s", VR_GetVRInitErrorAsEnglishDescription(eError));

  renderModels = (IVRRenderModels *)VR_GetGenericInterface(IVRRenderModels_Version, &eError);
  if (!renderModels) {
    VR_Shutdown();
    die("Unable to get render model interface.\n%s", VR_GetVRInitErrorAsEnglishDescription(eError));
  }
}

string openvr_tracker::driver() {
  return get_tracked_device_string(hmd, k_unTrackedDeviceIndex_Hmd, Prop_TrackingSystemName_String);
}

string openvr_tracker::serial_number() {
  return get_tracked_device_string(hmd, k_unTrackedDeviceIndex_Hmd, Prop_SerialNumber_String);
}

openvr_tracker::~openvr_tracker() {
  VR_Shutdown();
}

