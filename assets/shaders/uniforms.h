#ifndef INCLUDED_SHADERS_UNIFORMS_H_gasdlhad
#define INCLUDED_SHADERS_UNIFORMS_H_gasdlhad

// #define HACK_SEASCAPE

#ifdef __cplusplus
#include "framework/glm.h"

namespace framework {

  typedef GLuint64 samplerCube;

#define UNIFORM_STRUCT(n) struct
#define UNIFORM_ALIGN(n) __declspec(align(n))
#else
#define UNIFORM_STRUCT(n) layout (std140, binding = n) uniform
#define UNIFORM_ALIGN(n)
#endif

// 2^-10
#define FP16_SCALE 0.0009765625f

#define MAX_TRACKED_DEVICES 16
#define MAX_EYES 2
#define MAX_CONTROLLERS 2
#define DEVICE_HEAD 0

// monolithic ubo for common scene info, mostly sorted by size
UNIFORM_STRUCT(1) app_uniforms {  
  // head
  mat4 projection[MAX_EYES];
  mat4 inverse_projection[MAX_EYES];
  mat4 head_to_eye[MAX_EYES]; // affine
  mat4 eye_to_head[MAX_EYES]; // affine

  // derived world to eye
  mat4 current_pmv[MAX_EYES]; // convenience
  mat4 predicted_pmv[MAX_EYES]; // convenience

  // pose
  mat4 current_world_to_head;   // affineInverse(current_device_to_world[DEVICE_HEAD])
  mat4 predicted_world_to_head; // affineInverse(predicted_device_to_world[DEVICE_HEAD])
  
  mat4 current_device_to_world[MAX_TRACKED_DEVICES]; // affine
  mat4 predicted_device_to_world[MAX_TRACKED_DEVICES]; // affine

  mat4 current_controller_to_world[MAX_CONTROLLERS]; // affine
  mat4 predicted_controller_to_world[MAX_CONTROLLERS]; // affine

  // sky
  UNIFORM_ALIGN(16) vec3 sun_dir;
  UNIFORM_ALIGN(16) vec3 sun_color;
  UNIFORM_ALIGN(16) samplerCube sky_cubemap;
  UNIFORM_ALIGN(8)  float cos_sun_angular_radius;

  // camera
  float bloom_exposure;
  float bloom_magnitude;
  float exposure;
  float global_time;
  float nearClip;
  float farClip;

  // pose info, shuffled down here by size
  int controller_mask;
  int controller_device[MAX_CONTROLLERS];
};

#ifdef __cplusplus
} // namespace uniform
#endif

#endif // INCLUDED