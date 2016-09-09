#ifndef INCLUDED_SHADERS_LENS_FLARE_GLSL
#define INCLUDED_SHADERS_LENS_FLARE_GLSL

const vec3 flare_purple = vec3(0.7f, 0.2f, 0.9f);
const vec3 flare_orange = vec3(0.7f, 0.4f, 0.2f);

vec3 flare(sampler2DArray tex, vec2 uv, int samples, vec3 tint, float tex_scale, float blur_scale, float layer, float threshold) {
  vec2 source_dim = vec2(200, 220);
  vec2 mirror_coord = vec2(resolve_buffer_usage, resolve_buffer_usage) - uv;
  vec2 normalized_coord = (mirror_coord * (2.0f / resolve_buffer_usage) - 1.0f) / 1.3;
  normalized_coord *= tex_scale;
  vec2 toward_center = normalize(-normalized_coord);
  float blur_dist = blur_scale * samples;
  vec2 offset = (toward_center / source_dim) * blur_dist;
  vec2 start_point = normalized_coord - 5 * offset;
  vec2 step = -2 * offset;
  vec4 sum = vec4(0);
  vec2 sample_pos = start_point;
  for (int i = 0; i < samples; ++i) {
    vec2 sample_tex_coord = sample_pos * resolve_buffer_usage / 2 + resolve_buffer_usage / 2;
    float l = length(sample_tex_coord / resolve_buffer_usage * 2 - 1);
    if (l <= 1) {
      vec4 sampled = texture(tex, vec3(sample_tex_coord, layer));
      float weight = pow(1.0 - l, 20.0);
      sum += max(vec4(0), sampled - threshold) * vec4(tint, 1) * weight;;
      
    }
    sample_pos += step;
  }
  if (sum.w >= 0.5) return sum.xyz / sum.w; // amples;
  else return vec3(0);
}

#endif