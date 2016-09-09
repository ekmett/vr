#ifndef INCLUDED_SHADERS_COLOR_GLSL
#define INCLUDED_SHADERS_COLOR_GLSL

float luminance(vec3 rgb) {
  const vec3 kY = vec3(0.2126, 0.7152, 0.0722);
  return max(dot(rgb, kY), 0.0001);
}

float linearDepth(float depth, mat4 projMatrix) {
  return projMatrix[3][2] / (depth - projMatrix[2][2]);
}

// Grzegorz Krawczyk's dynamic key function
float autokey(in float lum) {
	return 1.03 - 2.0 / (2.0 + log2(lum + 1.0)/log2(10.0f));
}


// ALU driven approximation of Duiker's film stock curve
// by Jim Hejl and Richard Burgess-Dawson.
// NB: This has a baked in gamma correction
vec3 filmic(vec3 color) {
  color = max(vec3(0), color - vec3(0.004f));
  color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);
  return color;
}

#endif