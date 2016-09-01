#ifndef INCLUDED_SHADERS_BLUR_H_dakjsdhla
#define INCLUDED_SHADERS_BLUR_H_dakjsdhla

// Calculates the gaussian blur weight for a given distance and sigma
float gaussian_weight(int sample_distance, float sigma) {
  float g = 1.0f / sqrt(2.0f * 3.14159f * sigma * sigma);
  return g * exp(-(sample_distance * sample_distance) / (2 * sigma * sigma));
}

// Performs a gaussian blur in one diection
vec4 blur(sampler2D tex, vec2 input_coord, float2 texScale, float sigma, bool normalized) {
  vec2 input_size = textureSize(texture0);
  vec4 color = 0;
  float weights = 0.0f;
  for (int i = -7; i < 7; i++) {
    float weight = gaussian_weight(i, sigma);
    weights += weight;
    float2 texCoord = input_coord;
    texCoord += (i / input_size) * texScale;
    color += weight * texture(tex, texCoord);
  }

  if (normalized)
    color /= weightSum;

  return color;
}

#endif