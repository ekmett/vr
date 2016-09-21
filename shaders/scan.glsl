#ifndef INCLUDED_SCAN_GLSL
#define INCLUDED_SCAN_GLSL

// Utilities for performing prefix-scans in compute shaders

// #extension GL_NV_shader_thread_group : require
// #extension GL_NV_shader_thread_shuffle : require

#define THREADBLOCK_SIZE 512
#define BATCH_SIZE 2048u
#define LOG2_WARP_SIZE 5u
#define WARP_SIZE (1u << LOG2_WARP_SIZE)

layout(local_size_x = THREADBLOCK_SIZE) in;
shared uint shared_data[(THREADBLOCK_SIZE / WARP_SIZE)];

uint warp_scan_inclusive(uint d, uint N) {
  for (uint i = 1, i_max = min(N >> 1, 5u);i < i_max; i <<= 1) {
    bool valid = false;
    uint t = shuffleUpNV(d, i, 32, valid);
    if (valid) d += t;
  }
  return d;
}

uint warp_scan_exclusive(uint d, uint N) {
  return warp_scan_inclusive(d, N) - d;
}

uint scan1_inclusive(uint d, uint N) {
  if (N <= WARP_SIZE)
    return warp_scan_inclusive(d, N);

  uint warp_result = warp_scan_inclusive(d, WARP_SIZE);
  uint thread_index = gl_LocalInvocationID.x;

  if ((thread_index & (WARP_SIZE - 1)) == (WARP_SIZE - 1))
    shared_data[thread_index >> LOG2_WARP_SIZE] = warp_result;

  memoryBarrierShared();
  barrier();

  if (thread_index < THREADBLOCK_SIZE / WARP_SIZE)
    shared_data[thread_index] = warp_scan_exclusive(shared_data[thread_index], N >> LOG2_WARP_SIZE);

  memoryBarrierShared();
  barrier();

  return warp_result + shared_data[thread_index >> LOG2_WARP_SIZE];
}

uint scan1_exclusive(uint d, uint N) {
  return scan1_inclusive(d, N) - d;
}

vec4 scan4_inclusive(uvec4 d, uint N) {
  d.y += d.x; d.z += d.y; d.w += d.z;
  return d + scan1_exclusive(d.w, N >> 2);
}

vec4 scan4_exclusive(uvec4 d, uint N) {
  return scan4_inclusive(d, N) - d;
}

#endif