
#version 450
out vec4 color;
in vec3 cellColor;

void main() {
  color = vec4(cellColor, 1.0);
}