#version 450 core       
#extension GL_ARB_shading_language_include : require
#pragma optionNV(unroll all)

#include "uniforms.h"
#include "post_blur.glsl"

uniform sampler2DArray input;

in vec3 coord;

out vec4 outputColor;

void main() {
  outputColor = blur(input, coord, vec2(1,0,0), blur_sigma, false);
}