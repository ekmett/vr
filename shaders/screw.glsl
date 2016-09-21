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
dquat sclerp(dquat x, dquat y, float a) {
  float dp = dotq(x.r, y.r);
  screw s = make_screw(mul(conjugate(x), dp < 0 ? mul(y, -1) : y));
  return mul(x, make_dquat(pow_screw(s, a)));
}

#endif