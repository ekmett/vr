#version 450
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require
#extension GL_AMD_vertex_shader_layer : require

#include "uniforms.h"

layout(location = 0) uniform mat4 component_matrix = mat4(1.0);
layout(location = 1) uniform int device;

layout(location = 0) in vec4 positionIn;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec2 uvIn;

out vec2 uv;
out vec3 N;
out vec3 L;

/*
mat4 affine_inverse(mat4 m) {
  mat3 m3inv = inverse(mat3(m));
  return mat4( 
    vec4(m3inv[0],          0), // col 0
    vec4(m3inv[1],          0), // col 1
    vec4(m3inv[2],          0), // col 2 
    vec4(-m3inv * m[3].xyz, 1)  // col 3
  );
}
*/

void main() {
  uv = uvIn;

  mat4 P = projection[gl_InstanceID];
  mat4 V = head_to_eye[gl_InstanceID] * predicted_world_to_head;
  mat4 M = predicted_device_to_world[device] * component_matrix;

  mat4 IV = predicted_device_to_world[DEVICE_HEAD] * eye_to_head[gl_InstanceID];
  mat4 IP = inverse_projection[gl_InstanceID];
  
  N = normalIn * inverse(mat3(M) * mat3(component_matrix)) * mat3(IV) * mat3(IP); // normal in camera space

  vec3 position_in_cameraspace = (V*(M*positionIn)).xyz;
  vec3 eye_dir_in_cameraspace = vec3(0) - position_in_cameraspace;

  L = mat3(V) * sun_dir - position_in_cameraspace;

  gl_Position = predicted_pmv[gl_InstanceID] * predicted_device_to_world[device] * component_matrix * vec4(positionIn.xyz, 1);
  gl_Layer = gl_InstanceID;
}