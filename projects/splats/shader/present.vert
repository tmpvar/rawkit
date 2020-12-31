#version 450

const vec3 positions[3] = vec3[3](
  vec3(-1.f,-1.f, 0.0f),
  vec3(-1.f,4.f, 0.0f),
  vec3(4.f,-1.f, 0.0f)
);

out vec2 uv;

void main() {
  uv = positions[gl_VertexIndex].xy * 0.5 + 0.5;

	gl_Position = vec4(positions[gl_VertexIndex], 1.0);
}