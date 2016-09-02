#version 450 core
#extension GL_ARB_shader_viewport_layer_array : require

const vec2 positions[4] = vec2[](vec2(-1.0,1.0),vec2(-1.0,-1.0),vec2(1.0,1.0),vec2(1.0,-1.0));
const vec2 coords[4] = vec2[](vec2(0.0,1.0),vec2(0.0,-1.0),vec2(1.0,1.0),vec2(1.0,0.0));

noperspective out vec2 coord;

void main() {
  gl_ViewportIndex = gl_InstanceID;
  vec2 p = positions[gl_VertexId];
  gl_Position = vec4(p,0.0,1.0);
  coord = clamp(p,0.0,1.0);
}