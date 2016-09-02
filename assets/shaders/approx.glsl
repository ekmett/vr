#ifndef INCLUDED_SHADERS_APPROX_H_hlasdfjhl
#define INCLUDED_SHADERS_APPROX_H_hlasdfjhl

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif

// assumes 0 <= x
float fast_atan_pos(float x) {
  vec3 A = x < 1 ? vec3(x, 0, 1) : vec3(1 / x, 0.5 * M_PI, -1);
  return A.y + A.z * (((-0.130234 * A.x - 0.0954105) * A.x + 1.00712) * A.x - 0.00001203333);
}

// assumes -1 <= x <= 1
float fast_atan_small(float x) {
  float ax = abs(x);
  return M_PI_4 * x - x * (ax - 1) * (0.2447 + 0.0663*ax);
}

#endif