#ifndef INCLUDED_SHADERS_OCT_H_adsflha
#define INCLUDED_SHADERS_OCT_H_adsflha

// octahedron normal vectors
// [Cigolle 2014, "A Survey of Efficient Representations for Independent Unit Vectors"](http://jcgt.org/published/0003/02/01/)
vec2 unit_to_oct(vec3 N) {
  N.xy /= dot(1, abs(N));
  if (N.z <= 0)
    N.xy = (1 - abs(N.yx)) * (N.xy >= 0 ? vec2(1, 1) : vec2(-1, -1));
  return N.xy;
}

vec3 oct_to_unit(vec2 oct) {
  vec3 N = vec3(oct, 1 - dot(1, abs(oct)));
  if (N.z < 0)
    N.xy = (1 - abs(N.yx)) * (N.xy >= 0 ? vec2(1, 1) : vec2(-1, -1));
  return normalize(N);
}

#endif