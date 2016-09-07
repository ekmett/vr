#ifndef INCLUDED_SHADERS_SPHERICAL_HARMONICS_GLSL
#define INCLUDED_SHADERS_SPHERICAL_HARMONICS_GLSL

vec3 eval_sh9_irradiance(vec3 dir, vec4 sh[9]) {
  const float pi = 3.14159f;
  const float cosA0 = pi;
  const float cosA1 = (2.0f  * pi) / 3.0f;
  const float cosA2 = pi / 4.0f;

  return
    // band 0
    cosA0 * 0.282095f * sh[0].xyz +
    // band 1
    cosA1 * 0.488603f * (
      dir.y * sh[1].xyz +
      dir.z * sh[2].xyz +
      dir.x * sh[3].xyz
      ) +
    // band 2
    cosA2 * (
      1.092548f * (
        dir.x * dir.y * sh[4].xyz +
        dir.y * dir.z * sh[5].xyz +
        dir.x * dir.z * sh[7].xyz
      ) +
      0.315392f * (3.0f * dir.z * dir.z - 1.0f) * sh[6].xyz +
      0.546274f * (dir.x * dir.x - dir.y * dir.y) * sh[8].xyz
    );
}

#endif