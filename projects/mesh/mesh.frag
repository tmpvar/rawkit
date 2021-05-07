#version 450
layout(location = 0) out vec4 fragColor;
in vec4 vColor;
void main() {
  fragColor = vec4(vColor.xyz, 1.0);
}