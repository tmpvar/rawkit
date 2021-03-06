//we will be using glsl version 4.5 syntax
#version 450

out vec2 uv;

const vec3 positions[3] = vec3[3](
  vec3(-1.f,-1.f, 0.0f),
  vec3(-1.f,4.f, 0.0f),
  vec3(4.f,-1.f, 0.0f)
);

void main() {
  uv = positions[gl_VertexIndex].xy * 0.5 + 0.5;
	gl_Position = vec4(positions[gl_VertexIndex], 1.0);
}