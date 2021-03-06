#version 450 core
#extension GL_ARB_shading_language_include : require
#extension GL_ARB_bindless_texture : require
#include "uniforms.h"

layout (bindless_sampler, location = 0) uniform sampler2DArray last_resolve;
layout (location = 1) uniform float last_resolve_buffer_usage;

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
  if (fBoundsCheck >= 1.0f) outputColor = vec4(0.18f,0.18f,0.18f,0.0);
  else {
    vec4 r = texture(last_resolve, vec3(red * last_resolve_buffer_usage, eye));
    vec4 g = texture(last_resolve, vec3(green * last_resolve_buffer_usage, eye));
    vec4 b = texture(last_resolve, vec3(blue * last_resolve_buffer_usage, eye));
    outputColor = vec4(r.r, g.g, b.b,1);    
    //outputColor = mix(vec4(0.18f, 0.18f, 0.18f,0.0f), vec4(r.r,g.g,b.b,1), smoothstep(1, 3, r.a + g.a + b.a));
  }
}