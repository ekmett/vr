#ifndef INCLUDED_SHADERS_BLUR_H
#define INCLUDED_SHADERS_BLUR_H

// Calculates the Gaussian blur weight for a given distance and sigma
float gaussian_weight(int sample_distance, float sigma) {
  float sigma2 = sigma * sigma;
  float g = 1.0f / sqrt(2.0f * 3.14159f * sigma2);
  return g * exp(-(sample_distance * sample_distance) / (2 * sigma2));
}

// Performs a Gaussian blur in one direction
vec4 blur(sampler2DArray tex, vec3 input_coord, vec2 tex_scale, float sigma, bool normalized) {
  vec2 input_size = textureSize(tex, 0).xy;
  vec2 delta = tex_scale / input_size;
  vec3 color = vec3(0);
  float weights = 0.0f;
  for (int i = -6; i < 7; i++) {
    float weight = gaussian_weight(i, sigma);
    vec3 tex_coord = input_coord;
    tex_coord.xy += i * delta; 
    vec4 s = texture(tex, tex_coord);
    color += weight * s.rgb;
    weights += s.a * weight;
  }

  if (normalized)
    color /= weights;

  vec4 here = texture(tex, input_coord);

  if (here.a >= 0.5) return vec4(clamp(color,0,65000), here.a);
  else return here;
}

#endif
