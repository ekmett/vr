#version 450 core
#extension GL_AMD_vertex_shader_layer : enable

const vec2 positions[4] = vec2[](vec2(-1.0,1.0),vec2(-1.0,-1.0),vec2(1.0,1.0),vec2(1.0,-1.0));

noperspective out vec3 coord;

void main() {
  vec2 p = positions[gl_VertexId];
  gl_Position = vec4(p,0.0,1.0);
  gl_Layer = gl_InstanceID;
  coord = clamp(p,0.0,1.0);
}