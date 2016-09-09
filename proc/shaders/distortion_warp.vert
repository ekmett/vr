#version 450 core
#extension GL_ARB_shading_language_include : require
#extension GL_ARB_bindless_texture : require
#include "uniforms.h"

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 redIn;
layout(location = 2) in vec2 greenIn;
layout(location = 3) in vec2 blueIn;
layout(location = 4) in int eyeIn;

noperspective out vec2 red;
noperspective out vec2 green;
noperspective out vec2 blue;
flat out int eye;

void main() {
  red = redIn;
  green = greenIn;
  blue = blueIn;
  eye = eyeIn;
  gl_Position = position;
}