#ifndef INCLUDED_AABB_GLSL
#define INCLUDED_AABB_GLSL

#include "sphere.glsl"

struct aabb {
  vec3 lo, hi;
};

aabb make_aabb(vec3 p) {
  return aabb(p, p);
}

// compute an axis aligned bounding box for a sphere.
aabb make_aabb(sphere s) {
  return aabb(s.data.xyz - s.data.w, s.data.xyz + s.data.w);
}

// compute a bounding sphere for a bounding box
sphere make_sphere(aabb box) {
  return make_sphere(0.5 * (box.lo + box.hi), 0.5 * distance(box.lo, box.hi));
}

bool overlaps(aabb b, vec3 p) {
  return all(lessThan(b.lo, p)) && all(lessThan(p, b.hi));
}

bool overlaps(vec3 p, aabb b) {
  return overlaps(b, p);
}

aabb union_aabb(aabb a, aabb b) {
  return aabb(min(a.lo, b.lo), max(a.hi, b.hi));
}

aabb union_aabb(aabb a, vec3 p) {
  return aabb(min(a.lo, p), max(a.hi, p));
}

aabb union_aabb(aabb a, sphere s) {
  return union_aabb(a, make_aabb(s));
}

#endif