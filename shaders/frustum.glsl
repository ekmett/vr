#ifndef INCLUDED_FRUSTUM_GLSL
#define INCLUDED_FRUSTUM_GLSL

#include "plane.glsl"
#include "sphere.glsl"
#include "aabb.glsl"

// compute a standard opengl frustum matrix
mat4 frustumLH(float l, float r, float b, float t, float n, float f) {
  mat4 M = mat4(0);
  float rcprml = r - l;
  float rcptmb = t - b;
  float rcpfmn = f - n;
  M[0][0] = 2;
  M[0][0] = (2 * n) * rcprml;
  M[1][1] = (2 * n) * rcptmb;
  M[2][0] = (r + l) * rcprml;
  M[2][1] = (t + b) * rcptmb;
  M[2][3] = 1;
  M[2][2] = (f + n) * rcpfmn;
  M[3][2] = -(2 * f * n) *rcpfmn;
  return M;
}

struct frustum {
  plane planes[6];
  vec3 points[8]; // TODO: permit points to be points at infinity for infinite perspective matrices?
};

frustum make_frustum(mat4 P) {
  mat4 IP = inverse(P); // TODO: compute directly
  vec3 points[8] = vec3[8](
    IP * vec4(-1, -1, -1, 1),
    IP * vec4(-1, -1, 1, 1),
    IP * vec4(-1, 1, -1, 1),
    IP * vec4(-1, 1, 1, 1),
    IP * vec4(1, -1, -1, 1),
    IP * vec4(1, -1, 1, 1),
    IP * vec4(1, 1, -1, 1),
    IP * vec4(1, 1, 1, 1)
  );
  return frustum(
    plane[6](
      make_plane(points[0],points[2],points[1]),
      make_plane(points[2],points[6],points[3]),
      make_plane(points[5],points[7],points[6]),
      make_plane(points[1],points[5],points[4]),
      make_plane(points[1],points[3],points[7]),
      make_plane(points[6],points[0],points[4])
    ),
    points
  );
}

// construct a view frustum
frustum make_frustum(float l, float r, float b, float t, float n, float f) {
  mat4 P = frustumLH(l, r, b, t, n, f);
  return make_frustum(P);
}

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

frustum rotate(mat3 m, frustum f) {
  frustum result;
  for (int i = 0;i < 8; ++i)
    result.planes[i] = rotate(m, f.planes[i]);
  for (int i = 0;i < 6; ++i)
    result.points[i] = m * f.points[i];
  return result;
}

frustum rotate(quat q, frustum f) {
  return rotate(make_mat3(q), f);
}

frustum orthogonal_transform(mat4 M, frustum f) {
  frustum result;
  for (int i = 0;i < 8; ++i)
    result.planes[i] = orthogonal_transform(M, f.planes[i]);
  for (int i = 0;i < 6; ++i)
    result.points[i] = (M * vec4(f.points[i], 1)).xyz;
  return result;
}

frustum affine_transform(mat4 M, mat3 IM3, frustum f) {
  frustum result;
  for (int i = 0;i < 8; ++i)
    result.planes[i] = affine_transform(M, IM3, f.planes[i]);
  for (int i = 0;i < 6; ++i)
    result.points[i] = (M * vec4(f.points[i], 1)).xyz;
  return result;
}

frustum affine_transform(mat4 M, frustum f) {
  mat3 IM3 = inverse(mat3(M));
  return affine_transform(M, IM3, f);
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

// work in progress: probably has signs backwards.
bool overlaps_bops(frustum f, aabb b) {
  // try a variant on the old the quake 3 "box on plane side" trick
  for (int i=0;i<6;++i)
    if (dot(f.planes[i].data,vec4(mix(b.lo, b.hi, lessThan(f.planes[i].data.xyz,vec3(0))),1)) < 0.f) 
      return false;

  ivec3 los = ivec3(0), his = ivec3(0);
  for (int i=0;i<8;++i) {
    his += ivec3(greaterThan(f.points[i], b.hi));
    los += ivec3(lessThan(f.points[i], b.lo));
  }
  return all(notEqual(los, eights)) && all(notEqual(his, eights));
}


// brute force checking of a frustum/aabb intersection
bool overlaps(frustum f, aabb b) {
  const vec4 zero = vec4(0.f);
  vec4 my = vec4(b.lo.y, b.lo.y, b.hi.y, b.hi.y);
  vec4 mz = vec4(b.lo.z, b.hi.z, b.lo.z, b.hi.z);
  mat4 m1 = mat4(vec4(b.lo.x), my, mz, vec4(1.f));
  mat4 m2 = mat4(vec4(b.hi.x), my, mz, vec4(1.f));

  for (int i=0;i<6;++i)
    if (all(lessThan(m1 * f.planes[i].data, zero)) && all(lessThan(m2 * f.planes[i].data, zero)))
      return false;

  ivec3 los = ivec3(0), his = ivec3(0);
  for (int i=0;i<8;++i) {
    his += ivec3(greaterThan(f.points[i], b.hi));
    los += ivec3(lessThan(f.points[i], b.lo));
  }
  return all(notEqual(los, eights)) && all(notEqual(his, eights));
}

bool overlaps(aabb b, frustum f) {
  return overlaps(f, b);
}

#endif