#version 450 core       
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require

#include "uniforms.h"

// #include "post.h"

layout(bindless_sampler, location = 0) uniform sampler2DArray presolve;
layout(bindless_sampler, location = 1) uniform sampler2DArray bloom;

in vec3 coord;

out vec4 outputColor;

// ALU driven approximation of Duiker's film stock curve
vec3 filmic(vec3 color) {
  color = max(vec3(0), color - vec3(0.004f));
  color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);
  return color;
}

void main() {
  vec3 color = texture(presolve, coord).rgb;
  color += texture(bloom, coord).rgb; // * bloom_magnitude * exp2(bloom_exposure);
  color *= exp2(exposure);// / FP16_SCALE;                    
  color = color / (color + 1);
  // color = filmic(color);
  color = pow(color, vec3(1/2.2f));
  outputColor = vec4(color, 1.0f);
}
