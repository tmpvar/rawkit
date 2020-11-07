#version 450

layout (location = 0) in vec3 aPosition;

out vec4 vColor;

layout (std430, binding = 0) uniform UBO {
  vec4 color;
  mat4 mvp;
} uniforms;

layout (std430, set = 1, binding = 0) uniform Offsets {
  vec4 vec[10];
} offsets;

void main() {
  vec3 offset = offsets.vec[gl_InstanceIndex].xyz;
  vColor = vec4(float(gl_InstanceIndex) / 10.0) + 0.25;
  // vColor = vec4(1.0, 0.0, 1.0, 1.0);
	gl_Position = uniforms.mvp * vec4(aPosition + offset , 1.0);
}