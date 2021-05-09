#version 450

layout (location = 0) out vec4 outColor;
layout (binding = 1) uniform sampler2D tex;

in vec2 uv;

void main() {
  outColor = texture(tex, uv);
}