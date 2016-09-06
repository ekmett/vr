#version 450
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require
#extension GL_AMD_vertex_shader_layer : require

#include "uniforms.h"

layout(location = 0) uniform mat4 component_matrix = mat4(1.0);
layout(location = 1) uniform int device;

layout(location = 0) in vec4 positionIn;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec2 uvIn;

out vec2 uv;
out vec3 N;
out vec3 I;

void main() {
  uv = uvIn;
  mat4 M = predicted_device_to_world[device] * component_matrix;
  I = positionIn.xyz - (predicted_device_to_world[DEVICE_HEAD] * (eye_to_head[gl_InstanceID] * vec4(0,0,0,1))).xyz;
  N = mat3(M) * normalIn; // M^-1T = M so long as it doesn't scale
  gl_Position = predicted_pmv[gl_InstanceID] * (M * vec4(positionIn.xyz, 1));
  gl_Layer = gl_InstanceID;
}