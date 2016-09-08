#version 450 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require

#include "uniforms.h"

vec3 getSkyColor(vec3 dir) {
  vec3 color = texture(sky_cubemap, dir).xyz;
  if (cos_sun_angular_radius > 0.0f) {
    float cos_sun_angle = dot(dir, sun_dir);
    if (cos_sun_angle >= cos_sun_angular_radius)
      if (dir.y > 0) color = sun_color;
  }
  return color;
}


#include "seascape.glsl"

in vec3 origin;
in vec3 coord;

out vec4 outputColor;

void main() {
  vec3 dir = normalize(coord);
  vec3 color = getSkyColor(dir);
  if (enable_seascape != 0) {
    vec3 p = heightMapTracing(origin,dir);
    vec3 dist = p - origin;
    vec3 n = getNormal(p, dot(dist,dist) * 0.1 / viewport_w);
    color = mix(color, getSeaColor(p,n,sun_dir,dir,dist), pow(smoothstep(0.00,-0.05,dir.y),0.3));
    // gl_FragDepth = length(dist); // use GL_ARB_conservative_depth for seascape?
  }
  outputColor = vec4(color,1); 
}

