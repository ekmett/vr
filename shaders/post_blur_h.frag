#version 450 core       
#extension GL_ARB_shading_language_include : require
#extension GL_ARB_bindless_texture : require
#pragma optionNV(unroll all)

#include "uniforms.h"
#include "post_blur.glsl"

layout(bindless_sampler, location = 0) uniform sampler2DArray tex;

in vec3 coord;

out vec4 outputColor;

void main() {
  outputColor = blur(tex, coord, vec2(1,0), blur_sigma * resolve_buffer_usage, true);
}