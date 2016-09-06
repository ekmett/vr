#version 450 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require

#include "uniforms.h"

layout (bindless_sampler, location = 2) uniform sampler2D diffuse;

in vec2 uv;
in vec3 N;
in vec3 L;
out vec4 outputColor;
void main() {
  vec4 color = pow(texture(diffuse, uv), vec4(2.2));
  float a = 0.5;
  float d = 0.08;
  outputColor = vec4(color.xyz * (a + max(0, dot(normalize(N), normalize(L))) * sun_color * d), color.a);

}