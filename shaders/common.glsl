#ifndef INCLUDED_SHADERS_COMMON_H
#define INCLUDED_SHADERS_COMMON_H

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


#endif