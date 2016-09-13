#ifndef INCLUDED_SHADERS_COMMON_H
#define INCLUDED_SHADERS_COMMON_H

const float e = 2.7182818284590452354f;       // e
const float m_2_pi = 0.63661977236758134308f; // 2/pi
const float pi = 3.14159265358979323846f;     // pi
const float pi_2 = 1.57079632679489661923f;   // pi/2
const float pi_4 = 0.78539816339744830962f;   // pi/4
const float tau = 2 * pi;

#define saturate(x) clamp(x,0.f,1.f)

float sqr(float x) {
  return x*x;
}

vec2 sqr(vec2 x) {
  return x*x;
}

vec3 sqr(vec3 x) {
  return x*x;
}

vec4 sqr(vec4 x) {
  return x*x;
}

float cos2sin(float c) {
  return sqrt(1 - c*c);
}

vec2 cos2sin(vec2 c) {
  return sqrt(1 - c*c);
}

vec3 cos2sin(vec3 c) {
  return sqrt(1 - c*c);
}

vec4 cos2sin(vec4 c) {
  return sqrt(1 - c*c);
}

float sin2cos(float s) {
  return sqrt(1 - s*s);
}

vec2 sin2cos(vec2 s) {
  return sqrt(1 - s*s);
}

vec3 sin2cos(vec3 s) {
  return sqrt(1 - s*s);
}

vec4 sin2cos(vec4 s) {
  return sqrt(1 - s*s);
}

#endif