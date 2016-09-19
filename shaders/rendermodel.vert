#version 450
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require
#extension GL_AMD_vertex_shader_layer : require

#include "uniforms.h"

layout(location = 0) uniform mat4 component_matrix = mat4(1.0);
layout(location = 1) uniform int device;

layout(location = 0) in vec3 positionIn;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec2 uvIn;

out vec2 uv;
out vec3 N_ws;
out vec3 V_ws;

void main() {
  uv = uvIn;
  mat4 M = device == -1 ? component_matrix : predicted_device_to_world[device] * component_matrix;
  vec3 camera_ws = (predicted_device_to_world[DEVICE_HEAD] * eye_to_head[gl_InstanceID][3]).xyz;
  V_ws = camera_ws - positionIn.xyz;
  N_ws = mat3(M) * normalIn; // NB: (M^-1)^T = M if no components scale
  gl_Position = predicted_pmv[gl_InstanceID] * M * vec4(positionIn, 1);
  gl_Layer = gl_InstanceID;
}