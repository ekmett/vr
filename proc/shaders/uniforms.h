#ifndef INCLUDED_SHADERS_UNIFORMS_H_gasdlhad
#define INCLUDED_SHADERS_UNIFORMS_H_gasdlhad

#ifdef __cplusplus
#include "framework/glm.h"

namespace uniform {
  using namespace glm;

#define BEGIN_UNIFORM_BLOCK(x) struct x {
#define END_UNIFORM_BLOCK(x) };
#else
#define BEGIN_UNIFORM_BLOCK(x) layout (std14) uniform x {
#define END_UNIFORM_BLOCK(x) } x;
#endif

#define MAX_TRACKED_DEVICES 16

  BEGIN_UNIFORM_BLOCK(Head)
    mat4 perspective[2];
    mat4 inversePerspective[2];
    mat4 headToEye[2];
    mat4 eyeToHead[2];
  END_UNIFORM_BLOCK(head)

  BEGIN_UNIFORM_BLOCK(Pose)
    mat4 headToWorld;
    mat4 worldToHead;
    mat4 tracked[16];
    mat4 predicted[16];
    mat4 controller[2];
  END_UNIFORM_BLOCK(pose);

#ifdef __cplusplus
} // namespace uniform
#endif

#endif // INCLUDED