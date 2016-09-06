#version 450 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require

#include "uniforms.h"

layout (bindless_sampler, location = 2) uniform sampler2D diffuse_texture;

in vec2 uv;
in vec3 N;
in vec3 I;
out vec4 outputColor;
void main() {
  vec3 L = sun_dir;
  vec4 d = pow(texture(diffuse_texture, uv), vec4(2.2));
  vec3 diffuse = d.xyz * max(0, dot(normalize(N), L)); // * sun_color;
  vec3 ambient = d.xyz * texture(sky_cubemap, reflect(I,N)).xyz;
  outputColor = vec4(ambient*0.4 + diffuse*0.6, d.a);
}