#ifndef INCLUDED_SHADERS_APPROX_GLSL_hlasdfjhl
#define INCLUDED_SHADERS_APPROX_GLSL_hlasdfjhl

#include "common.glsl"

// assumes 0 <= x
float fast_atan_pos(float x) {
  vec3 A = x < 1 ? vec3(x, 0, 1) : vec3(1 / x, 0.5 * pi, -1);
  return A.y + A.z * (((-0.130234 * A.x - 0.0954105) * A.x + 1.00712) * A.x - 0.00001203333);
}

// assumes -1 <= x <= 1
float fast_atan_small(float x) {
  float ax = abs(x);
  return pi_4 * x - x * (ax - 1) * (0.2447 + 0.0663*ax);
}

#endif