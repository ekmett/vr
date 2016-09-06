#version 450 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require

#include "uniforms.h"

#include "seascape.glsl"

in vec3 origin;
in vec3 coord;

out vec4 outputColor;

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 11.2;
 
vec3 uncharted2_tonemap(vec3 x) {
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main() {
  vec3 color = texture(sky_cubemap, coord).xyz;
  vec3 dir = normalize(coord);
  if (cos_sun_angular_radius > 0.0f) {
    float cos_sun_angle = dot(dir, sun_dir);
    if (cos_sun_angle >= cos_sun_angular_radius)
      if (dir.y > 0) color = sun_color; //horizon. this doesn't properly account for loss of radiance.
  }

  //if (enable_seascape != 0) {
    vec3 p = heightMapTracing(origin,dir);
    vec3 dist = p - origin;
    vec3 n = getNormal(p, dot(dist,dist) * EPSILON_NRM);
    color = mix(color, getSeaColor(p,n,sun_dir,dir,dist), pow(smoothstep(0.00,-0.15,dir.y),0.3)); 
 // }
  outputColor = vec4(color,1);
  // outputColor = vec4(coord, 1);
  // outputColor = vec4(1,0,1,1);
}

