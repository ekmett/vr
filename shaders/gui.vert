#version 450

layout(location = 0) uniform mat4 projection;

layout (location = 0) in vec2 positionIn;
layout (location = 1) in vec2 uvIn;
layout (location = 2) in vec4 colorIn;

noperspective out vec2 uv;
noperspective out vec4 color;

void main() {
  uv = uvIn;
  color = colorIn;
  gl_Position = projection * vec4(positionIn.xy,0,1);
}