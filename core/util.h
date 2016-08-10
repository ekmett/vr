#pragma once

#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <openvr.h>
#include <Windows.h>

const char * show_debug_source(GLenum source);
const char * show_debug_message_type(GLenum type);
const char * show_tracked_device_class(vr::TrackedDeviceClass c);
const char * show_event_type(vr::EVREventType e);
const char * show_chaperone_calibration_state(vr::ChaperoneCalibrationState state);

void objectLabelf(GLenum id, GLuint name, const char *fmt, ...);
__declspec(noreturn) void die_helper(const char * function, const char *fmt, ...);

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

static inline vr::EVREye eye(int i) {
  return (i == 0) ? vr::Eye_Left : vr::Eye_Right;
}

static inline const char * label_eye(int i) {
  switch (i) {
  case 0: return "left";
  case 1: return "right";
  default: return "unknown";
  }
}

static inline std::wstring from_utf8(const char * s) {
  std::wstring ret;
  int slen = (int)strlen(s);
  int dlen = MultiByteToWideChar(CP_UTF8, 0, s, slen, NULL, 0);
  if (dlen > 0) {
    ret.resize(dlen);
    MultiByteToWideChar(CP_UTF8, 0, s, slen, const_cast<wchar_t*>(ret.c_str()), dlen);
  }
  return ret;
}

#define die(...) die_helper(__FUNCTION__, __VA_ARGS__)