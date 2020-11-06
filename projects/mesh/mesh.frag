#version 450
layout(location = 0) out vec3 fragColor;
in vec4 vColor;
void main() {
  fragColor = vColor.xyz;
  // fragColor = vec3(1.0, 0.0, 1.0);
}