#version 450
out vec4 outColor;

in vec3 rayOrigin;
in vec3 normal;
flat in vec3 eye;
flat in uint brick_id;

void main() {

  // if (normal.y > 0.0) {
  //   outColor = vec4(1.0);
  // } else if (normal.x != 0.0 || normal.z != 0.0) {
  //   outColor = vec4(0.7s);
  // } else {
  //   outColor = vec4(0.2);
  // }

  outColor = vec4(normal, 1.0);
}