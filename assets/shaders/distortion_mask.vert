#version 450 core
#extension GL_AMD_vertex_shader_layer : enable

layout(location = 0) uniform int right_eye_threshold;

layout(location = 0) in vec4 position;
// in int gl_VertexID:
void main() {
  gl_Position = position;
  gl_Layer = (gl_VertexID >= right_eye_threshold) ? 1 : 0;
}