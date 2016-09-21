#ifndef INCLUDED_DUAL_QUAT_GLSL
#define INCLUDED_DUAL_QUAT_GLSL

#include "quat.glsl"

// -----------------------------------------------------------------------------
// * Dual Quaternions
// -----------------------------------------------------------------------------

// A decent introduction to dual quaternions can be found in
// [Dual Quaternions: From Classical Mechanics to Computer Graphics and Beyond](http://www.xbdev.net/misc_demos/demos/dual_quaternions_beyond/paper.pdf)
// by Ben Kenwright

// 32 bytes (8 floats), representing a 6-dof rigid body transformation.
// (subject to 2 constraints)
struct dquat {
  quat r, d; // primal (real) and dual components
};

dquat make_dquat(quat r, quat d) {
  return dquat(r, d);
}

dquat make_dquat() {
  return dquat(make_quat(1, 0, 0, 0), make_quat(0, 0, 0, 0));
}

// build a dual quaternion from a quaternion rotation
dquat make_dquat(quat q) {
  return dquat(q, make_quat(0,0,0,0));
}

// dual quaternion representation of a vector
dquat make_dquat(vec3 v) {
  return dquat(make_quat(1, 0, 0, 0), make_quat(0, v));
}

// build a dual quaternion from a quaternion rotation and a translation vector
dquat make_dquat(quat q, vec3 p) {
  return dquat(q, make_quat(
    -0.5 * (p.x*q.data.x + p.y*q.data.y + p.z*q.data.z),
     0.5 * (p.x*q.data.w + p.y*q.data.z - p.z*q.data.y),
     0.5 * (-p.x*q.data.z + p.y*q.data.w + p.z*q.data.x),
     0.5 * (p.x*q.data.y - p.y*q.data.x + p.z*q.data.w)  
  ));
}

// construct a dual quaternion from a rotation/translation matrix
dquat make_dquat(mat3x4 x) {
  // assume mat3(x) is orthogonal
  quat real;
  float trace = x[0][0] + x[1][1] + x[2][2];
  if (trace > 0) {
    float r = sqrt(1 + trace);
    float rcpr = 0.5 / r;
    real.data = vec4(
      (x[2].y - x[1].z) * rcpr,
      (x[0].z - x[2].x) * rcpr,
      (x[1].x - x[0].y) * rcpr,
      0.5 * r
    );
  } else if (x[0][0] > x[1][1] && x[0][0] > x[2][2]) {
    float r = sqrt(1 + x[0].x - x[1].y - x[2].z);
    float rcpr = 0.5 / r;
    real.data = vec4(
      0.5 * r,
      (x[1].x + x[0].y) * rcpr,
      (x[0].z + x[2].x) * rcpr,
      (x[2].y - x[1].z) * rcpr
    );
  } else if (x[1].y > x[2].z) {
    float r = sqrt(1 + x[1].y - x[0].x - x[2].z);
    float rcpr = 0.5 / r;
    real.data = vec4(
      (x[1].x + x[0].y) * rcpr,
      0.5 * r,
      (x[2].y + x[1].z) * rcpr,
      (x[0].z - x[2].x) * rcpr
    );
  } else {
    float r = sqrt(1 + x[2].z - x[0].x - x[1].y);
    float invr = 0.5 / r;
    real.data = vec4(
      (x[0].z + x[2].x) * invr,
      (x[2].y + x[1].z) * invr,
      0.5 * r,
      (x[1].x - x[0].y) * invr
    );
  }
  return dquat(real, make_quat(
    -0.5 * ( x[0].w * real.data.x + x[1].w * real.data.y + x[2].w * real.data.z),
     0.5 * ( x[0].w * real.data.w + x[1].w * real.data.z - x[2].w * real.data.y),
     0.5 * (-x[0].w * real.data.z + x[1].w * real.data.w + x[2].w * real.data.x),
     0.5 * ( x[0].w * real.data.y - x[1].w * real.data.x + x[2].w * real.data.w)
  ));
}

dquat make_dquat(mat4 x) {
  // assume mat3(x) is orthogonal
  // assume row 3 of x = 0 0 0 1
  return make_dquat(mat3x4(x));
}

// construct a pure translation dual quaternion
dquat make_dquat_translation(vec3 t) {
  return dquat(make_quat(1, 0, 0, 0), make_quat(0, 0.5*t));
}

dquat add(dquat a, dquat b) {
  return dquat(add(a.r, b.r), add(a.d, b.d));
}

dquat mul(dquat a, dquat b) {
  return dquat(mul(a.r, b.r), add(mul(a.r, b.d), mul(a.d, b.r)));
}

dquat mul(dquat a, quat b) {
  return dquat(mul(a.r, b), mul(a.d, b));
}

dquat mul(quat a, dquat b) {
  return dquat(mul(a, b.r), mul(a, b.d));
}

dquat mul(dquat a, float b) {
  return dquat(mul(a.r, b), mul(a.d, b));
}

dquat div(dquat a, float b) {
  return dquat(div(a.r, b), div(a.d, b));
}

dquat normalizeq(dquat a) {
  return div(a, quadrance(a.r));
}

dquat conjugate(dquat a) {
  return dquat(conjugate(a.r), conjugate(a.d));
}

float dotq(dquat a, dquat b) {
  return dotq(a.r, b.r) + dotq(a.d, b.d);
}


float quadrance(dquat a) {
  return dotq(a, a);
}

float lengthq(dquat a) {
  return sqrt(quadrance(a));
}

vec3 translation(dquat a) {
  return mul(mul(a.d, 2), conjugate(a.r)).data.xyz;
}

// compute the inverse dual quaternion
dquat inverseq(dquat a) {
  quat r = conjugate(a.r);
  quat d = conjugate(a.d);
  return dquat(r, add(d, mul(r, -2 * dotq(r, d))));
}

// construct an rotation/translation matrix from a dual quaternion
mat3x4 make_mat3x4(dquat x) {
  quat r = div(x.r, quadrance(x.r));
  vec4 rr = r.data * x.r.data;
  r.data *= 2;

  float xy = r.data.x * x.r.data.y;
  float xz = r.data.x * x.r.data.z;
  float yz = r.data.y * x.r.data.z;
  float wx = r.data.w * x.r.data.x;
  float wy = r.data.w * x.r.data.y;
  float wz = r.data.w * x.r.data.z;

  return mat3x4(
    vec3(
      rr.w + rr.x - rr.y - rr.z,                                                     //0,0
      xy + wz,                                                                       //1,0
      xz - wy                                                                        //2,0
    ),
    vec3(
      xy - wz,                                                                       //0,1
      rr.w + rr.y - rr.x - rr.z,                                                     //1,1
      yz + wx                                                                        //2,1
    ),
    vec3(
      xz + wy,                                                                       //0,2
      yz - wx,                                                                       //1,2
      rr.w + rr.z - rr.x - rr.y                                                      //2,2
    ),
    vec3(
      -(x.d.data.w * r.data.x - x.d.data.x * r.data.w + x.d.data.y * r.data.z - x.d.data.z * r.data.y),  //0,3  
      -(x.d.data.w * r.data.y - x.d.data.x * r.data.z - x.d.data.y * r.data.w + x.d.data.z * r.data.x),  //1,3
      -(x.d.data.w * r.data.z + x.d.data.x * r.data.y - x.d.data.y * r.data.x - x.d.data.z * r.data.w)   //2,3
    )
  );
}

// construct an rotation/translation matrix from a dual quaternion
mat4 make_mat4(dquat x) {
  quat r = div(x.r, quadrance(x.r));
  vec4 rr = r.data * x.r.data;
  r.data *= 2;

  float xy = r.data.x * x.r.data.y;
  float xz = r.data.x * x.r.data.z;
  float yz = r.data.y * x.r.data.z;
  float wx = r.data.w * x.r.data.x;
  float wy = r.data.w * x.r.data.y;
  float wz = r.data.w * x.r.data.z;

  return mat4(
    vec4(
      rr.w + rr.x - rr.y - rr.z,                                                     //0,0
      xy + wz,                                                                       //1,0
      xz - wy,                                                                       //2,0
      0
    ),
    vec4(
      xy - wz,                                                                       //0,1
      rr.w + rr.y - rr.x - rr.z,                                                     //1,1
      yz + wx,                                                                       //2,1
      0
    ),
    vec4(
      xz + wy,                                                                       //0,2
      yz - wx,                                                                       //1,2
      rr.w + rr.z - rr.x - rr.y,                                                     //2,2
      0
    ),
    vec4(
      -(x.d.data.w * r.data.x - x.d.data.x * r.data.w + x.d.data.y * r.data.z - x.d.data.z * r.data.y),  //0,3 
      -(x.d.data.w * r.data.y - x.d.data.x * r.data.z - x.d.data.y * r.data.w + x.d.data.z * r.data.x),  //1,3
      -(x.d.data.w * r.data.z + x.d.data.x * r.data.y - x.d.data.y * r.data.x - x.d.data.z * r.data.w),  //2,3
      1
    )
  );
}

// dual quaternion linear blend (DLB)
dquat lerp(dquat x, dquat y, float a) {
  // assume 0 <= a <= 1
  // take the shortest path
  float k = dotq(x.r, y.r) < 0 ? -a : a;
  return add(mul(x, 1 - a), mul(y, k));
}

#endif