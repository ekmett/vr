#version 450
// #extension GL_ARB_bindless_texture : require

layout (binding = 0) uniform sampler2D tex;

in vec2 uv;
in vec4 color;

out vec4 outputColor;

void main() {
  outputColor = color * texture(tex, uv.st);
}