#version 450
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require
#extension GL_ARB_shader_viewport_layer_array : enable
#include "uniforms.h"
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color_in;
layout(location = 2) in int controller;
out vec4 color;
void main() {
  color = vec4(color_in, 1.0f);
  gl_Position = predicted_pmv[gl_InstanceID] * (predicted_controller_to_world[controller] * position);
  gl_ViewportIndex = gl_InstanceID;
}
