#version 450

layout (location = 0) in vec3 aPosition;

out vec4 color;

void main() {

  color = vec4(float(gl_VertexIndex) / 6);
	gl_Position = vec4(aPosition, 1.0f);
}