#version 450 core       
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require

#include "uniforms.h"
#include "color.glsl"

layout(bindless_sampler, location = 0) uniform sampler2DArray presolve;
layout(bindless_sampler, location = 1) uniform sampler2DArray bloom;
layout(bindless_sampler, location = 2) uniform sampler1D lenscolor;
layout(bindless_sampler, location = 3) uniform sampler2D lensdirt;
layout(bindless_sampler, location = 4) uniform sampler2D lensstar;

in vec3 coord;

out vec4 outputColor;

vec2 fudge = vec2(0,0.01);

// add these to app_uniforms as mat2s?

vec2 ndc_to_uv(vec2 ndc) {
  return (ndc + 1.f) * 0.5f;
}

vec2 uv_to_ndc(vec2 uv) {
  return uv * 2 - 1;
}

vec2 uv_to_resolve(vec2 uv) {
  return uv * resolve_buffer_usage;
}

vec2 resolve_to_uv(vec2 resolve) {
  return resolve / resolve_buffer_usage;
}

vec2 ndc_to_resolve(vec2 ndc) {
  return (ndc + 1.f) * (resolve_buffer_usage / 2.f) + fudge;
}

vec2 resolve_to_ndc(vec2 resolve) {
  return (resolve - fudge) * (2.f / resolve_buffer_usage) - 1.f;
}

vec2 mirror_resolve(vec2 resolve) {
  return vec2(resolve_buffer_usage) - coord.xy;
}

vec2 mirror_uv(vec2 uv) {
  return 1 - uv;
}

vec2 mirror_ndc(vec2 ndc) {
  return -ndc;
}

const float threshold = 0.1;

// todo: factor in faux chromatic aberration
vec3 fetch(vec2 ndc) {
  return max(texture(bloom, vec3(ndc_to_resolve(ndc),coord.z)).rgb - threshold,0.0) / (1 - threshold);
}

vec3 flare3() {
  vec2 uv = coord.xy / resolve_buffer_usage;
  vec2 ndc = uv_to_ndc(uv);
  vec2 dg_ndc = ndc * lens_flare_ghost_dispersal;
  vec3 result = vec3(0);
  for (int i = 0;i < lens_flare_ghosts; ++i) {
    vec2 sample_ndc = -ndc + dg_ndc * pow(i,1.1);
    float len = length(sample_ndc)/sqrt(2);
    if (len < 1) {
      result += fetch(sample_ndc) * pow(1-len,20);
    }       
  }
  vec2 halo_ndc = ndc - normalize(dg_ndc) * lens_flare_halo_radius;
  result += fetch(halo_ndc) * pow(length(halo_ndc),2.5);
  return result * texture(lenscolor, length(ndc)/sqrt(3)).rgb;  
}


void main() {
  vec4 presolve_color = texture(presolve, coord);
  if (presolve_color.a < 0.1) { // leave the edges untouched
    outputColor = vec4(presolve_color.xyz, 1);
  } else {
    vec3 color = presolve_color.rgb;
    color += texture(bloom, coord).rgb * bloom_magnitude * exp2(bloom_exposure);
    if (use_lens_flare != 0) {
      vec2 uv = coord.xy / resolve_buffer_usage;
      int eye = coord.z >= 0.5 ? 1 : 0;
      mat3 view = mat3(head_to_eye[eye]) * mat3(predicted_world_to_head);
      float camrot = dot(view[0], vec3(0,0,1)) + dot(view[2], vec3(0,1,0));
      float c = cos(camrot);
      float s = sin(camrot);
      mat2 rot = mat2(c,-s, s, c);    
      vec3 lensmod =
        texture(lensdirt,uv + eye).rgb + 
        texture(lensstar, ndc_to_uv(rot * uv_to_ndc(uv))).rgb;

      color += flare3().rgb * lensmod *  exp2(lens_flare_exposure);
    }
    color *= exp2(exposure) / FP16_SCALE;
    color = filmic(color);
    outputColor = vec4(color.rgb, 1);  
  }
}
