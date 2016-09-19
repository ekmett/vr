#ifndef INCLUDED_FRUSTUM_GLSL
#define INCLUDED_FRUSTUM_GLSL

#include "plane.glsl"
#include "sphere.glsl"
#include "aabb.glsl"

struct frustum {
  plane planes[6];
  vec3 points[8]; // TODO: permit points to be points at infinity for infinite perspective matrices?
};

// construct a 'frustum' from an axis aligned bounding box.
frustum make_frustum(aabb b) {
  return frustum(
    plane[6](
      make_plane(-1,0,0,-b.lo.x),
      make_plane(1,0,0,b.hi.x),
      make_plane(0,-1,0,-b.lo.y),
      make_plane(0,1,0,b.hi.y),
      make_plane(0,0,-1,-b.lo.z),
      make_plane(0,0,1,b.hi.z)
    ),
    vec3[8](
      vec3(b.lo.x, b.lo.y, b.lo.z),
      vec3(b.lo.x, b.lo.y, b.hi.z),
      vec3(b.lo.x, b.hi.y, b.lo.z),
      vec3(b.lo.x, b.hi.y, b.hi.z),
      vec3(b.hi.x, b.lo.y, b.lo.z),
      vec3(b.hi.x, b.lo.y, b.hi.z),
      vec3(b.hi.x, b.hi.y, b.lo.z),
      vec3(b.hi.x, b.hi.y, b.hi.z)
    )
  );
}

const ivec3 eights = ivec3(8, 8, 8);

// Compute a fairly-accurate accurate sphere-frustum check
bool overlaps(frustum f, sphere s) {
  vec3 center = sphere_center(s);
  float radius = sphere_radius(s);

  // test sphere against frustum
  for (int i=0;i<6;++i)
    if (signed_distance(f.planes[i], center) < -radius)
      return false;

  // Q: is it worth including a fast acceptance test here for strict containment?
  
  // test frustum against the bounding box of sphere.
  ivec3 los = ivec3(0);
  ivec3 his = ivec3(0);
  vec3 min = center - radius;
  vec3 max = center + radius;

  for (int i=0;i<8;++i) {
    his += ivec3(greaterThan(f.points[i], max));
    los += ivec3(lessThan(f.points[i], min));
  }

  // Q: is it worth doing finer-grained testing with the actual sphere?
  return all(notEqual(los, eights)) && all(notEqual(his, eights));
}

bool overlaps(sphere s, frustum f) {
  return overlaps(f, s);
}

bool overlaps(frustum f, aabb b) {
  const vec4 zero = vec4(0.f);
  vec4 my = vec4(b.lo.y, b.lo.y, b.hi.y, b.hi.y);
  vec4 mz = vec4(b.lo.z, b.hi.z, b.lo.z, b.hi.z);
  mat4 m1 = mat4(vec4(b.lo.x), my, mz, vec4(1.f));
  mat4 m2 = mat4(vec4(b.hi.x), my, mz, vec4(1.f));

  for (int i=0;i<6;++i)
    if (all(lessThan(m1 * f.planes[i].data, zero)) && all(lessThan(m2 * f.planes[i].data, zero)))
      return false;

  ivec3 los = ivec3(0);
  ivec3 his = ivec3(0);
  for (int i = 0;i < 8;++i) {
    his += ivec3(greaterThan(f.points[i], b.hi));
    los += ivec3(lessThan(f.points[i], b.lo));
  }
  return all(notEqual(los, eights)) && all(notEqual(his, eights));
}

bool overlaps(aabb b, frustum f) {
  return overlaps(f, b);
}

#endif