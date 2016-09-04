#version 450 core
#extension GL_ARB_shading_language_include : require
#extension GL_ARB_bindless_texture : require
#include "uniforms.h"

uniform sampler2DArray resolve;

noperspective in vec2 red;
noperspective in vec2 green;
noperspective in vec2 blue;
flat in int eye;

out vec4 outputColor;

void main() {
  // does this add any accuracy?
  vec2 lo = min(min(red, green), blue);
  vec2 hi = max(max(red, green), blue);
  float fBoundsCheck = ( 
     (dot( vec2( lessThan( lo.xy, vec2(0.05, 0.05) ) ), vec2(1.0, 1.0))
    + dot( vec2( greaterThan( hi.xy, vec2( 0.95, 0.95) ) ), vec2(1.0, 1.0))) );
  if (fBoundsCheck >= 1.0f) outputColor = vec4(0.18f,0.18f,0.18f,1);
  else {
    float r = texture(resolve, vec3(red * resolve_buffer_usage, eye)).r;
    float g = texture(resolve, vec3(green * resolve_buffer_usage, eye)).g;
    float b = texture(resolve, vec3(blue * resolve_buffer_usage, eye)).b;      
    outputColor = vec4(r,g,b,1);
  }
}