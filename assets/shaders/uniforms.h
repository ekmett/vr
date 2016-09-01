#ifndef INCLUDED_SHADERS_UNIFORMS_H_gasdlhad
#define INCLUDED_SHADERS_UNIFORMS_H_gasdlhad

#ifdef __cplusplus
#include "framework/glm.h"

namespace uniform {
  using namespace glm;

#define UNIFORM_STRUCT(n) struct
#else
#define UNIFORM_STRUCT(n) layout (std140, binding = n) uniform
#endif

#define MAX_TRACKED_DEVICES 16

struct head {
  mat4 perspective[2];
  mat4 inversePerspective[2];
  mat4 headToEye[2];
  mat4 eyeToHead[2];
};

struct pose {
  mat4 headToWorld;
  mat4 worldToHead;
  mat4 tracked[16];
  mat4 predicted[16];
  mat4 controller[2];
};

struct sky {
  

}

#ifdef __cplusplus
} // namespace uniform
#endif

#endif // INCLUDED