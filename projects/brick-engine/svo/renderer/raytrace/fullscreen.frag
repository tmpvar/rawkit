#version 450

layout (location = 0) out vec4 outColor;
layout (binding = 1) uniform sampler2D tex_color;

void main() {
  outColor = texture(tex_color, gl_FragCoord.xy);
}