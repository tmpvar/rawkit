#version 450
out vec4 outColor;

in vec3 rayOrigin;
in vec3 normal;
flat in vec3 eye;
flat in uint brick_id;

void main() {
  outColor = vec4(normal, 1.0);
}