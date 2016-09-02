#version 450

uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 outputColor;

void main() {
  outputColor = Frag_Color * texture(Texture, Frag_UV.st);
}