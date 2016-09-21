#ifndef INCLUDED_QUAT_GLSL
#define INCLUDED_QUAT_GLSL

#include "common.glsl"

struct quat {
  vec4 data;
};

quat make_quat() {
  return quat(vec4(0,0,0,0));
}

quat make_quat(float w) {
  return quat(vec4(0, 0, 0, w));
}

quat make_quat(float w, vec3 xyz) {
  return quat(vec4(xyz, w));
}

quat make_quat(float w, float x, float y, float z) {
  return quat(vec4(x, y, z, w));
}

// build a quaternion from an orthogonal matrix
quat make_quat(mat3 m) {
  float fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
  float fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
  float fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
  float fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];
  int biggestIndex = 0;
  float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
  if (fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
    fourBiggestSquaredMinus1 = fourXSquaredMinus1;
    biggestIndex = 1;
  }
  if (fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
    fourBiggestSquaredMinus1 = fourYSquaredMinus1;
    biggestIndex = 2;
  }
  if (fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
    fourBiggestSquaredMinus1 = fourZSquaredMinus1;
    biggestIndex = 3;
  }
  float a = sqrt(fourBiggestSquaredMinus1 + 1) * 0.5;
  float mult = 0.25f / a;
  vec4 t = vec4(
    (m[1][2] - m[2][1]) * mult,
    (m[2][0] - m[0][2]) * mult,
    (m[0][1] - m[1][0]) * mult,
    a
  );
  switch (biggestIndex) {
    case 1: t = t.wzyx; break;
    case 2: t = t.zwxy; break;
    case 3: t = t.yxwz; break;
    default: break;
  }
  return quat(t);
}

// construct a quaternion that rotates by theta around axis. assumes axis is normalized
quat quat_from_axis_angle(vec3 axis, float theta) {
  float half_theta = 0.5 * theta;
  return make_quat(cos(half_theta), sin(half_theta) * axis);
}

// build a quaternion that rotates from o to d.
quat make_quat(vec3 o, vec3 d) {
  float cosTheta = dot(o, d);
  if (cosTheta >= 1 - epsilon) return make_quat(); // too close
  if (cosTheta <  epsilon - 1) { 
    // vectors point straight away from one another, pick a direction, favoring up.
    vec3 axis = cross(vec3(0, 0, 1), o);
    if (quadrance(axis) < epsilon)
      axis = cross(vec3(1, 0, 0), o);
    return quat_from_axis_angle(normalize(axis),pi);
  }
  vec3 axis = cross(o, d);
  float s = 2 * sqrt(1 + cosTheta);
  return make_quat(0.5 * s, axis / s);
}

// check 2 quaternions for equality
bool eq(quat q1, quat q2) {
  return q1.data == q2.data;
}

float dotq(quat q1, quat q2) {
  return dot(q1.data, q2.data);
}

// squared magnitude
float quadrance(quat q) {
  return quadrance(q.data);
}

// returns a unit quaternion
quat normalizeq(quat q) {
  float l = length(q.data);
  if (l <= 0) return make_quat(1); // error
  return quat(q.data * rcp(l));
}

// add 2 quaternions
quat add(quat q1, quat q2) {
  return quat(q1.data + q2.data);
}

// subtract 2 quaternions
quat sub(quat q1, quat q2) {
  return quat(q1.data - q2.data);
}

// quaternion scalar multiplication
quat mul(quat q, float s) {
  return quat(q.data*s);
}

quat div(quat q, float s) {
  return quat(q.data / s);
}

// scalar quaternion multiplication
quat mul(float s, quat q) {
  return quat(s*q.data);
}

// multiply 2 quaternions
quat mul(quat q1, quat q2) {
  return make_quat(
    q1.data.w * q2.data.w - q1.data.x * q2.data.x - q1.data.y * q2.data.y - q1.data.z * q2.data.z, // w
    q1.data.w * q2.data.x + q1.data.x * q2.data.w + q1.data.z * q2.data.y - q1.data.y * q2.data.z, // x
    q1.data.w * q2.data.y + q1.data.y * q2.data.w + q1.data.x * q2.data.z - q1.data.z * q2.data.x, // y
    q1.data.w * q2.data.z + q1.data.z * q2.data.w + q1.data.y * q2.data.x - q1.data.x * q2.data.y  // z
  );
}

// rotate a vector. named by analogy to applying a mat3, but isn't really multiplication.
vec3 mul(quat q, vec3 v) {
  // return v + 2.f * cross(cross(v, q.data.xyz) + q.data.w * v), q.data.xyz);
  vec3 qv = q.data.xyz;
  vec3 uv = cross(qv, v);
  return 2.f * (uv*q.data.w + cross(qv, uv)) + v;
}

vec4 mul(quat q, vec4 v) {
  return vec4(mul(q,v.xyz), v.w);
}

// conjugate a quaternion
quat conjugate(quat q) {
  return quat(vec4(-q.data.xyz, q.data.w));
}

// compute the inverse quaternion
quat inverseq(quat q) {
  return quat(vec4(-q.data.xyz, q.data.w) / quadrance(q));
}

vec3 mul(vec3 v, quat q) {
  return mul(inverseq(q), v);
}

// construct a quaternion from euler angles
quat quat_from_euler(vec3 euler) {
  vec3 half_euler = euler * 0.5;
  vec3 c = cos(half_euler);
  vec3 s = sin(half_euler); // cos2sin(c) = sqrt(1 - c*c)
  return make_quat(
    c.x * c.y * c.z + s.x * s.y * s.z, // w
    s.x * c.y * c.z - c.x * s.y * s.z, // x
    c.x * s.y * c.z + s.x * c.y * s.z, // y
    c.x * c.y * s.z - s.x * s.y * c.z  // z
  );
}

// exponentiate a quaternion
quat expq(quat q) {
  vec3 u = q.data.xyz;
  float theta = length(u);
  if (theta < epsilon) return quat(vec4(0.f));
  return make_quat(cos(theta), u * (sin(theta) / theta));
}

quat sqrtq(quat q) {
  vec3 v  = q.data.xyz;
  float w = q.data.w;
  float qv = quadrance(v);
  float m = sqrt(w*w + qv);

  // too close to 0
  if (m < epsilon) return q;

  // keep it real
  if (qv < epsilon) return w > 0 ? make_quat(sqrt(w), 0, 0, 0) : make_quat(0, sqrt(-w), 0, 0);

  return make_quat(0.5*(m + w), v * sqrt(0.5*(m - w) / qv));
}

// take the log of a quaternion
quat logq(quat q) {
  vec3 u = q.data.xyz;
  float w = q.data.w;
  float theta = length(u);
  const float inf = 1.f / 0.f;
  if (theta < epsilon)
    return w > 0 ? make_quat(w, u) // preserve u to keep -0
         : w < 0 ? make_quat(-w, pi, u.y, u.z) // preserve u to keep -0
                 : make_quat(inf, inf, inf, inf);
  else {
    float t = atan(theta, w) / theta;
    return quat(vec4(t * u, 0.5 * log(sqr(theta) + sqr(w))));
  }
}

quat powq(quat x, float y) {
  if (y > -epsilon && y < epsilon) return make_quat(1, 0, 0, 0);
  float mag = length(x.data);
  float w = x.data.w;
  float cosTheta = w / mag;
  if (abs(cosTheta) > 1 - epsilon && abs(cosTheta) < 1 + epsilon)
    return make_quat(pow(w, y), 0, 0, 0);
  float theta = acos(cosTheta);
  float ytheta = y*theta;
  float m = pow(mag, y-1);
  return quat(m * vec4(sin(ytheta) / sin(theta) * x.data.xyz, cos(ytheta) * mag));
}

// simple normalized linear interpolation
quat lerp(quat x, quat y, float a) {
  return normalizeq(quat(mix(x.data, y.data, a)));
}

// spherical linear interpolation
quat slerp(quat x, quat y, float a) {
  float cosTheta = dotq(x, y);
  if (cosTheta < 0) { // take the short path
    y = quat(-y.data);
    cosTheta = -cosTheta;
  }
  if (cosTheta > 1 - epsilon) {
    return lerp(x, y, a); // linear interpolate when close
  } else {
    float theta = acos(cosTheta), atheta = a*theta;
    return quat((sin(theta - atheta) * x.data + sin(atheta) * y.data) / sin(theta));
  }
}

// slerp
quat mixq(quat x, quat y, float a) {
  return slerp(x, y, a);
}

quat intermediate(quat p, quat c, quat n) {
  quat i = inverseq(c);
  return mul(expq(
     mul(add(logq(add(n,i)),logq(add(p,i))),-0.25f)
  ), c);
}

// Proper spherical spline quaternion interpolation originally due to 
// "Quaternion Calculus and Fast Animation" by Ken Shoemake.
// See [Quaternions, Interpolation and Animation](http://web.mit.edu/2.998/www/QuaternionReport1.pdf)
// by Dam, Koch and Lillholm for an accessible online definition.
quat squad(quat q1, quat q2, quat s1, quat s2, float h) {
  return slerp(slerp(q1, q2, h), slerp(s1, s2, h), 2 * (1 - h) * h);
}

// rotate q by theta around v.
quat rotate(quat q, vec3 v, float theta) {
  return mul(q, quat_from_axis_angle(normalize(v), theta));
}

float roll(quat q) {
  vec4 d = q.data;
  return atan(2.f * (d.x * d.y + d.w * d.z), d.w * d.w + d.x * d.x - d.y * d.y - d.z * d.z);
}

float pitch(quat q) {
  vec4 d = q.data;
  return atan(2.f * (d.y * d.z + d.w * d.x), d.w * d.w - d.x * d.x - d.y * d.y + d.z * d.z);
}

float yaw(quat q) {
  vec4 d = q.data;
  return asin(clamp(-2.f * (d.x * d.z - d.w * d.y), -1.f, 1.f));
}

// construct a rotation matrix from a quaternion
mat3 make_mat3(quat q) {
  float xx = q.data.x * q.data.x;
  float yy = q.data.y * q.data.y;
  float zz = q.data.z * q.data.z;
  float xz = q.data.x * q.data.z;
  float xy = q.data.x * q.data.y;
  float yz = q.data.y * q.data.z;
  float wx = q.data.w * q.data.x;
  float wy = q.data.w * q.data.y;
  float wz = q.data.w * q.data.z;
  return mat3(
    1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy),
    2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx),
    2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy)
  );
}

#endif