#ifndef INCLUDED_SCREW_GLSL
#define INCLUDED_SCREW_GLSL

#include "dual_quat.glsl"

// -----------------------------------------------------------------------------
// * Screw Coordinates
// -----------------------------------------------------------------------------

// TODO: split out Pluecker coordinates (dir & moment) into their own thing.
// (see my old thesis)

struct screw {
  vec3 dir;
  vec3 moment;
  float theta;
  float pitch;
};

screw make_screw(dquat q) {
  vec3 vr = q.r.data.xyz;
  float rcplvr = inversesqrt(quadrance(vr));
  float theta = 2 * acos(q.r.data.w);
  float pitch = (-2 * rcplvr) * q.d.data.w;
  vec3 dir = vr * rcplvr;
  vec3 moment = (q.d.data.xyz - dir * (pitch * q.r.data.w * 0.5)) * rcplvr;
  return screw(dir, moment, theta, pitch);
}

// exponentiate a screw coordinate
screw pow_screw(screw x, float y) {
  return screw(x.dir, x.moment, x.theta * y, x.pitch * y);
}

screw pow_screw(screw x, float y, float dy) {
  return screw(x.dir, x.moment, x.theta * y, x.pitch * y + x.theta * dy);
}

dquat make_dquat(screw s) {
  float half_theta = 0.5 * s.theta;
  float cos_half = cos(half_theta);
  float sin_half = sin(half_theta);
  return dquat(
    make_quat(cos_half, s.dir * sin_half),
    make_quat(-0.5 * s.pitch * sin_half, sin_half - s.theta * s.moment + 0.5 * s.pitch * cos_half * s.dir)
  );
}

// perform screw linear interpolation of dual quaternions
//
// for unit dual quaternions q1, q2, p:
//
// sclerp(mul(q1,p),mul(q2,p),a) = mul(sclerp(q1,q2,a),p)
// sclerp(mul(p,q1),mul(p,q2),a) = mul(p,sclerp(q1,q2,a))
dquat sclerp(dquat x, dquat y, float a) {
  float dp = dotq(x.r, y.r);
  screw s = make_screw(mul(conjugate(x), dp < 0 ? mul(y, -1) : y));
  return mul(x, make_dquat(pow_screw(s, a)));
}

// screw quadratic interpolation of dual quaternions
dquat scquad(dquat q1, dquat q2, dquat s1, dquat s2, float h) {
  return sclerp(sclerp(q1, q2, h), sclerp(s1, s2, h), 2 * (1 - h) * h);
}

dquat powq(dquat x, float y) {
  // TODO: check for pure translation
  return make_dquat(pow_screw(make_screw(x), y));
}

// raise a dual quaternion to a dual scalar (y + dy*epsilon).
dquat powq(dquat x, float y, float dy) {
  return normalizeq(make_dquat(pow_screw(make_screw(x), y, dy)));
}

// compute the square root of a dual quaternion
dquat sqrtq(dquat x) {
  return powq(x, 0.5);
}

// transform a point
vec3 transform(dquat q, vec3 p) {
  vec4 r = q.r.data, d = q.d.data, r2 = r*r;
  return vec3(
      2 * (-d.w * r.x + d.z * r.y - d.y * r.z + d.x * r.w)
        + p.x * (r2.x - r2.y - r2.z + r2.w)
    + 2 * p.y * (r.y * r.x - r.w * r.z)
    + 2 * p.z * (r.x * r.z + r.y * r.w),
      2 * (-d.z * r.x - d.w * r.y + d.x * r.z + d.y * r.w)
    + 2 * p.x * (r.x * r.y + r.z * r.w)
        + p.y * (-r2.x + r2.y - r2.z + r2.w)
    + 2 * p.z * (-r.x * r.w + r.z * r.y),
      2 * (d.y * r.x - d.x * r.y - d.w * r.z + d.z * r.w)
    + 2 * p.x * (r.x * r.z - r.y * r.w)
    + 2 * p.y * (r.x * r.w + r.z * r.y)
        + p.z * (-r2.x - r2.y + r2.z + r2.w)
  );
}

// rotate a vector, applyies only the real part
vec3 rotate(dquat q, vec3 v) {
  return mul(q.r, v);
}


#endif