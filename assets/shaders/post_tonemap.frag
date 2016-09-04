#version 450 core       
#extension GL_ARB_shading_language_include : require
#include "uniforms.h"
#include "post.h"

uniform sampler2D render;
uniform sampler2D bloom;

in vec3 coord;

out vec4 outputColor;

// ALU driven approximation of Duiker's film stock curve
vec3 filmic(vec3 color) {
  color = max(0, color - 0.004f);
  color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);
  return color;
}

void main() {
  vec3 color = texture(render, coord).rgb;
  color += texture(bloom, coord).rgb * bloom_magnitude * exp2(bloom_exposure);
  color *= exp2(exposure) / fp16_scale;                   

  // crush out all the joy in the image with the tonemap, TODO: use the bloom kernel area to get an area around the point for a Reinhard Y-centric baseline instead?
  outputColor = filmic(color);
}
