#version 450 core

#extension GL_ARB_shading_language_include: require
#extension GL_AMD_vertex_shader_layer : require
#extension GL_ARB_bindless_texture : require

#include "uniforms.h"

const vec2 positions[3] = vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
const vec2 corners[3]   = vec2[](vec2(0,0),vec2(2,0),vec2(0,2));

noperspective out vec3 coord;

out gl_PerVertex {
  vec4 gl_Position;
  int gl_Layer;
};

void main() {
  coord = vec3(corners[gl_VertexID],gl_InstanceID); //  * resolve_buffer_usage
  gl_Position = vec4(positions[gl_VertexID],0.0,1.0);
  gl_Layer = gl_InstanceID;
}