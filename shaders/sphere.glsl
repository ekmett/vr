#ifndef INCLUDED_SPHERE_GLSL
#define INCLUDED_SPHERE_GLSL

#include "plane.glsl"
#include "quat.glsl"

struct sphere {
  vec4 data;
};

sphere make_sphere(vec3 xyz) {
  return sphere(vec4(xyz, 0.f));
}

sphere make_sphere(vec3 xyz, float center) {
  return sphere(vec4(xyz, center));
}

vec3 sphere_center(sphere s) {
  return s.data.xyz;
}

float sphere_radius(sphere s) {
  return s.data.w;
}

sphere rotate(quat q, sphere s) {
  return make_sphere(mul(q, s.data.xyz), s.data.w);
}

bool overlaps(sphere a, sphere b) {
  return dot(sphere_center(a), sphere_center(b)) < sqr(sphere_radius(a) + sphere_radius(b));
}

bool overlaps(sphere a, vec3 p) {
  return quadrance(a.data.xyz, p) < sqr(a.data.w);
}

#endif