#ifndef INCLUDED_PLANE_GLSL
#define INCLUDED_PLANE_GLSL

#include "quat.glsl"

struct plane {
  vec4 data;
};

plane make_plane(float x, float y, float z, float d) {
  return plane(vec4(x, y, z, d));
}

plane make_plane(vec3 N, float d) {
  return plane(vec4(N, d));
}

// construct a plane from a normal and a point on the plane
plane make_plane(vec3 N, vec3 p) {
  return make_plane(N, -dot(N, p));
}

// construct a plane from a clockwise triangle within the plane,
plane make_plane(vec3 A, vec3 B, vec3 C) {
  vec3 N = normalize(cross(A-C,B-C));
  return make_plane(N, C);
}

float signed_distance(plane p, vec3 pt) {
  return dot(p.data, vec4(pt, 1.f));
}

vec3 plane_normal(plane p) {
  return p.data.xyz;
}

float plane_distance(plane p) {
  return p.data.w;
}

// mat3(M) must be an orthogonal matrix (all column/row vectors are length 1)
plane orthogonal_transform(mat4 M, plane p) {
  return make_plane(
    mat3(M)*p.data.xyz,
    (M * vec4(-p.data.w * p.data.xyz, 1)).xyz // compute a point on the new plane
  );
}

// assumes M is an affine matrix. and IM3 = inverse(mat3(M)). scaling is okay.
plane affine_transform(mat4 M, mat3 IM3, plane p) {
  return make_plane(p.data.xyz * IM3, (M * vec4(-p.data.w * p.data.xyz, 1)).xyz);
}

// assumes M is an affine matrix, scaling is okay.
plane affine_transform(mat4 M, plane p) {
  return affine_transform(M, inverse(mat3(M)), p);
}

// rotate by an orthogonal matrix (all column/ row vectors are length 1)
plane rotate(mat3 M, plane p) {
  return make_plane(M*p.data.xyz, p.data.w);
}

// rotate by a unit quaternion
plane rotateq(quat q, plane p) {
  return make_plane(mul(q, p.data.xyz), p.data.w);
}

#endif
