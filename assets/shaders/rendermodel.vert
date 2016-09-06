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

void main() {
  uv = uvIn;
  gl_Position = predicted_pmv[gl_InstanceID] * predicted_device_to_world[device] * component_matrix * vec4(positionIn.xyz, 1);
  gl_Layer = gl_InstanceID;
}