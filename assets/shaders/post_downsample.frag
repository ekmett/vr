#version 450 core       
#pragma optionNV(unroll all)

uniform sampler2DArray render;

in vec3 coord;

out vec4 outputColor;

void main() {
  vec4 reds  = textureGather(render, coord, 0);
  vec4 greens = textureGather(render, coord, 1);
  vec4 blues  = textureGather(render, coord, 2);
  vec3 result;
  for (int i=0;i<4;++i)
    result += vec3(reds[i], greens[i], blues[i]);  
  outputColor = vec4(result / 4.0f, 1.0f);
}