#ifndef INCLUDED_SHADERS_BLUR_H
#define INCLUDED_SHADERS_BLUR_H

// Calculates the gaussian blur weight for a given distance and sigma
float gaussian_weight(int sample_distance, float sigma) {
  float sigma2 = sigma * sigma;
  float g = 1.0f / sqrt(2.0f * 3.14159f * sigma2);
  return g * exp(-(sample_distance * sample_distance) / (2 * sigma2));
}

// Performs a gaussian blur in one direction
vec3 blur(sampler2DArray tex, vec3 input_coord, vec2 tex_scale, float sigma, bool normalized) {
  vec2 input_size = textureSize(tex, 0).xy;
  vec2 delta = tex_scale / input_size;
  vec3 color = vec3(0);
  float weights = 0.0f;
  for (int i = -7; i < 7; i++) { // -6?
    float weight = gaussian_weight(i, sigma);
    weights += weight;
    vec3 tex_coord = input_coord;
    tex_coord.xy += i * delta; 
    color += weight * texture(tex, tex_coord).xyz;
  }

  if (normalized)
    color /= weights;

  return color;
}

#endif
