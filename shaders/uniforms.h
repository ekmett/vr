#ifndef INCLUDED_SHADERS_UNIFORMS_H_gasdlhad
#define INCLUDED_SHADERS_UNIFORMS_H_gasdlhad

#define HACK_SEASCAPE

#ifdef __cplusplus
#include "glm.h"

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
  // render model utilities
  UNIFORM_ALIGN(4) float rendermodel_smoothness;
  UNIFORM_ALIGN(4) float rendermodel_metallic;
  UNIFORM_ALIGN(4) float rendermodel_ambient;
  UNIFORM_ALIGN(4) float rendermodel_albedo;
  UNIFORM_ALIGN(4) int use_sun_area_light_approximation;
  UNIFORM_ALIGN(4) int viewport_w, viewport_h;
  UNIFORM_ALIGN(4) int use_lens_flare;
  UNIFORM_ALIGN(4) int lens_flare_ghosts;
  UNIFORM_ALIGN(4) float lens_flare_exposure, lens_flare_halo_radius;
  UNIFORM_ALIGN(4) float lens_flare_ghost_dispersal;

  // head
  UNIFORM_ALIGN(16) mat4 projection[MAX_EYES];
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
  
  UNIFORM_ALIGN(16) vec4 current_device_velocity[MAX_TRACKED_DEVICES];
  UNIFORM_ALIGN(16) vec4 predicted_device_velocity[MAX_TRACKED_DEVICES];
  UNIFORM_ALIGN(16) vec4 current_device_angular_velocity[MAX_TRACKED_DEVICES];
  UNIFORM_ALIGN(16) vec4 predicted_device_angular_velocity[MAX_TRACKED_DEVICES];                                               // sky

  UNIFORM_ALIGN(16) vec3 sun_dir;
  UNIFORM_ALIGN(16) vec3 sun_color;
  UNIFORM_ALIGN(16) vec3 sun_irradiance;
  UNIFORM_ALIGN(16) vec3 ground_albedo;

  UNIFORM_ALIGN(16) vec4 sky_sh9[9]; // using vec4s so we can share
  UNIFORM_ALIGN(16) samplerCube sky_cubemap;

  UNIFORM_ALIGN(8) float turbidity;
  UNIFORM_ALIGN(4) float cos_sun_angular_radius
                       , sin_sun_angular_radius, sun_angular_radius;

  // camera
  UNIFORM_ALIGN(4) float bloom_exposure, bloom_magnitude, blur_sigma;
  UNIFORM_ALIGN(4) float exposure;
  UNIFORM_ALIGN(4) float nearClip, farClip;
  UNIFORM_ALIGN(4) float global_time;

  UNIFORM_ALIGN(4) int enable_seascape; // final resolve buffer is in srgb colorspace, not linear

  UNIFORM_ALIGN(4) float render_buffer_usage, resolve_buffer_usage;

  // pose info, shuffled down here by size
  UNIFORM_ALIGN(4) int controller_mask;
  UNIFORM_ALIGN(4) int device_mask;
  UNIFORM_ALIGN(4) int controller_device[MAX_CONTROLLERS];
};

#ifdef __cplusplus
} // namespace uniform
#endif

#endif