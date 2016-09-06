#version 450 core

in vec2 uv;
out vec4 outputColor;
void main() {
  outputColor = vec4(1 - uv,0,0);
  //outputColor = vec4(0.18f,0.98f,0.18f,0);
}