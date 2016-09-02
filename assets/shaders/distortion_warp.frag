#version 450 core

uniform sampler2D resolve;

noperspective in vec2 v2UVred;
noperspective in vec2 v2UVgreen;
noperspective in vec2 v2UVblue;

out vec4 outputColor;

void main() {
  // does this add any accuracy?
  vec2 lo = min(min(v2UVgreen, v2UVred), v2UVblue);
  vec2 hi = max(max(v2UVgreen, v2UVred), v2UVblue);
  float fBoundsCheck = ( (dot( vec2( lessThan( lo.xy, vec2(0.02, 0.02)) ), vec2(1.0, 1.0))+dot( vec2( greaterThan( hi.xy, vec2( 0.98, 0.98)) ), vec2(1.0, 1.0))) );
  if (fBoundsCheck >= 1.0f) outputColor = vec4(0,0,0,1);
  else {
    float red   = texture(resolve, v2UVred).x;
    float green = texture(resolve, v2UVgreen).y;
    float blue  = texture(resolve, v2UVblue).z;
    outputColor = vec4( red, green, blue, 1.0  ); 
  }
}