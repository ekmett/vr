#version 450 core       
#extension GL_ARB_shading_language_include : require
#include "uniforms.h"
#include "post_blur.glsl"

uniform sampler2D input;

in vec2 coord;

out vec4 outputColor;

void main() {
  outputColor = blur(input, coord, vec2(1,0), blur_sigma, false);       
}