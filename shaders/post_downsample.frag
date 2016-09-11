#version 450 core       
#extension GL_ARB_shading_language_include : require
#extension GL_ARB_shading_language_include : require
#extension GL_ARB_bindless_texture : require
#pragma optionNV(unroll all)

#include "uniforms.h"

#define MAX_RENDER_TARGETS 2

layout(bindless_sampler, location = 0) uniform sampler2DArray render;

in vec3 coord;

out vec4 outputColor;

void main() {
  vec4 reds   = textureGather(render, coord, 0);
  vec4 greens = textureGather(render, coord, 1);
  vec4 blues  = textureGather(render, coord, 2);
  vec4 alphas = textureGather(render, coord, 3);
  vec4 result = vec4(0);
  for (int i=0;i<4;++i)
    result += vec4(reds[i], greens[i], blues[i], alphas[i]);  
  result /= 4.0f;
  outputColor = result;
}