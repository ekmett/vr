#include "error.h"
#include "overlay.h"

using namespace vr;

overlay::overlay(string key, string name, int width, int height) {
  auto driver = VROverlay();
  if (!driver)
    die("overlay manager isn't running"); // openvr has some slow start problems!
  auto result = driver->CreateDashboardOverlay(key.c_str(), name.c_str(), &handle, &thumbnail_handle);
  if (result != VROverlayError_None)
    die("Unable to create dashboard overlay: {}", driver->GetOverlayErrorNameFromEnum(result));

  driver->SetOverlayWidthInMeters(handle, 1.0f);
  driver->SetOverlayInputMethod(handle, VROverlayInputMethod_Mouse);
  driver->SetOverlayTextureColorSpace(handle, ColorSpace_Gamma);
  driver->SetOverlayFromFile(handle, R"(d:\vr\assets\sandspline-img.png)"); // placeholder
  driver->SetOverlayFromFile(thumbnail_handle, R"(d:\vr\assets\sandspline-img.png)");
  HmdVector2_t scale { (float)width, (float)height };
  driver->SetOverlayMouseScale(handle, &scale);
  // driver->SetDashboardOverlaySceneProcess() ... no way to lock it to our process without knowing our own process id
  // we should be able to magically set our size using imgui's feedback mechanisms after the first frame
}

overlay::~overlay() {
  VROverlay()->DestroyOverlay(handle);
}
