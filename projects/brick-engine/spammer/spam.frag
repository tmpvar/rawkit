#version 450
out vec4 outColor;

in vec3 rayOrigin;
in vec3 normal;
in vec3 worldPos;
flat in vec3 eye;
flat in uint brick_id;
flat in vec3 brick_color;


void main() {
  outColor = vec4(
    brick_color,
    1.0
  );
}