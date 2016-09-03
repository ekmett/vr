#version 450 core
#extension GL_AMD_vertex_shader_layer : enable
// #extension GL_ARB_shader_viewport_layer_array : require
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require
#include "uniforms.h"

const vec2 positions[4] = vec2[](
  vec2(-1.0,1.0), vec2(-1.0,-1.0),
  vec2( 1.0,1.0), vec2( 1.0,-1.0)
);

out vec3 coord; 

#ifdef HACK_SEASCAPE
out vec3 origin;
#endif

void main() {
  // mat4 model_view = head_to_eye[gl_InstanceID] * predicted_world_to_head[gl_InstanceID];
  vec4 position = vec4(positions[gl_VertexID],0.0,1.0);
  mat4 inverse_model_view = predicted_device_to_world[DEVICE_HEAD] * eye_to_head[gl_InstanceID];
#ifdef HACK_SEASCAPE
  origin = predicted_device_to_world[DEVICE_HEAD][3].xyz;
  origin.y -= 3;
#endif
  coord = mat3(inverse_model_view) * (inverse_projection[gl_InstanceID] * position).xyz;
  gl_Layer = gl_InstanceID;
  // gl_ViewportIndex = gl_InstanceID;
  gl_Position = position;
}