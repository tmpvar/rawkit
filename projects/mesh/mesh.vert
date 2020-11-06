#version 450

layout (location = 0) in vec3 aPosition;

out vec4 vColor;

layout (std430, binding = 0) uniform UBO {
  vec4 color;
  mat4 mvp;
} uniforms;

void main() {
  vColor = vec4(float(gl_VertexIndex) / 6.0);
	gl_Position = uniforms.mvp * vec4(aPosition, 1.0);
}