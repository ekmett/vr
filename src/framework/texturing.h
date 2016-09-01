#pragma once

#include "framework/glm.h"

namespace framework {

  vec3 xys_to_direction(int x, int y, int s, int w, int h) {
    float u = ((x + 0.5f) / float(w)) * 2.0f - 1.0f;
    float v = ((y + 0.5f) / float(h)) * 2.0f - 1.0f;
    v *= -1.0f;
    vec3 dir = vec3(0, 0, 0);
    // +x, -x, +y, -y, +z, -z
    switch (s) {
      case 0: dir = normalize(vec3(1, v, -u)); break; // right
      case 1: dir = normalize(vec3(-1, v, u)); break;  // left
      case 2: dir = normalize(vec3(u, 1, -v)); break; // top
      case 3: dir = normalize(vec3(u, -1, v)); break;  // bottom
      case 4: dir = normalize(vec3(u, v, 1)); break; // back
      case 5: dir = normalize(vec3(-u, v, -1)); break;  // front
    }
    return dir;
  }

}