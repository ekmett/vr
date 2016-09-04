#version 450
#extension GL_ARB_bindless_texture : require

layout (bindless_sampler, location = 0)  uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 outputColor;

void main() {
  outputColor = Frag_Color * texture(Texture, Frag_UV.st);
}