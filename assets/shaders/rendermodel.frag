#version 450 core
#extension GL_ARB_bindless_texture : require

layout (bindless_sampler, location = 2) uniform sampler2D diffuse;

in vec2 uv;
out vec4 outputColor;
void main() {
  outputColor = texture(diffuse, uv);
}