#version 450 core
#extension GL_ARB_shading_language_include : require
#extension GL_ARB_bindless_texture : require
#include "uniforms.h"

uniform sampler2D resolve;

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
     (dot( vec2( lessThan( lo.xy, vec2(0.02, 0.02) ) ), vec2(1.0, 1.0))
    + dot( vec2( greaterThan( hi.xy, vec2( 0.98, 0.98) ) ), vec2(1.0, 1.0))) );
  if (fBoundsCheck >= 1.0f) outputColor = vec4(0.18f,0.18f,0.18f,1);
  else {
    vec2 a = vec2(eye, 0);
    vec2 m = vec2(0.5 * resolve_buffer_usage, resolve_buffer_usage);
    float r = texture(resolve, (red + a) * m).r;
    float g = texture(resolve, (green + a) * m).g;
    float b = texture(resolve, (blue + a) * m).b;
      
    outputColor = vec4(r,g,b,1);
  }
}