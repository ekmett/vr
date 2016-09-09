#version 450 core       
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require

#include "uniforms.h"
#include "color.glsl"

layout(bindless_sampler, location = 0) uniform sampler2DArray presolve;
layout(bindless_sampler, location = 1) uniform sampler2DArray bloom;
layout(bindless_sampler, location = 2) uniform sampler1D flare1d;

in vec3 coord;

out vec4 outputColor;

vec2 fudge = vec2(0,0.01);

vec2 ndc_to_resolve(vec2 ndc) {
  return (ndc + 1.f) * (resolve_buffer_usage / 2.f) + fudge;
}

vec2 resolve_to_ndc(vec2 resolve) {
  return (resolve - fudge) * (2.f / resolve_buffer_usage) - 1.f;
}

vec2 mirror_resolve(vec2 resolve) {
  return vec2(resolve_buffer_usage) - coord.xy;
}

vec3 flare(int samples, float blur_dist, float layer, float threshold) {
  vec2 uv = coord.xy;
  vec2 mirror_coord = mirror_resolve(uv);
  vec2 normalized_coord = resolve_to_ndc(mirror_coord);
  vec2 toward_center = -normalized_coord;
  vec2 offset = toward_center / samples * blur_dist;
  vec2 step_ndc = -2 * offset;
  vec2 sample_ndc = normalized_coord;
  vec4 sum = vec4(0);
  for (int i = 0; i < samples; ++i) {
    vec2 sample_resolve = ndc_to_resolve(sample_ndc * 0.4);
    float len = length(sample_ndc) / sqrt(2);
    if (len <= 1) {
      vec4 sampled = texture(bloom, vec3(sample_resolve, layer));
      sum += sampled * vec4(texture(flare1d, len).rgb, 1) * pow(1-len,4);
    }
    sample_ndc += step_ndc;
  }
  return sum.w >= 0.1 ? sum.xyz / sum.w : vec3(0);
}

const float flare_exposure = -12;

void main() {
  vec4 presolve_color = texture(presolve, coord);
  if (presolve_color.a < 0.1) { // leave the edges untouched
    outputColor = vec4(presolve_color.xyz, 1);
  } else {
    vec3 color = presolve_color.rgb;
    color += texture(bloom, coord).rgb * bloom_magnitude * exp2(bloom_exposure);
    color += flare(6,3.5f, coord.z, 0.92f) * exp2(flare_exposure);
    color *= exp2(exposure) / FP16_SCALE;
    color = filmic(color);    
    outputColor = vec4(color.xyz, 1);  
  }
}
