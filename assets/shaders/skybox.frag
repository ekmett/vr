#version 450 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require
#include "uniforms.h"

#ifdef HACK_SEASCAPE
#include "seascape.glsl"
#endif

in vec3 origin;
in vec3 coord;

out vec4 outputColor;

void main() {
  vec3 color = texture(sky_cubemap, coord).xyz;
  vec3 dir = normalize(coord);
  if (cos_sun_angular_radius > 0.0f) {
    float cos_sun_angle = dot(dir, sun_dir);
    if (cos_sun_angle >= cos_sun_angular_radius)
      color = sun_color;
  }

#ifdef HACK_SEASCAPE
  vec3 p = heightMapTracing(origin,dir);
  vec3 dist = p - origin;
  vec3 n = getNormal(p, dot(dist,dist) * EPSILON_NRM);
  color = mix(color, getSeaColor(p,n,sun_dir,dir,dist), pow(smoothstep(-0.00,-0.15,dir.y),0.3));
#endif

  color = color / (color + 1);
  outputColor = vec4(color,1);
}

