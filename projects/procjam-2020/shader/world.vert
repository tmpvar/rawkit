#version 450

#include "../shared.h"

layout (std430, binding = 1) uniform UBO {
  world_ubo_t data;
} ubo;


out vec3 rayOrigin;

// a cube
const vec3 positions[36] = vec3[36](
  vec3(-1.0, -1.0, 1.0),
  vec3(1.0, 1.0, 1.0),
  vec3(1.0, -1.0, 1.0),

  vec3(-1.0, -1.0, 1.0),
  vec3(-1.0, 1.0, 1.0),
  vec3(1.0, 1.0, 1.0),

  vec3(1.0, 1.0, 1.0),
  vec3(1.0, 1.0, -1.0),
  vec3(1.0, -1.0, -1.0),

  vec3(1.0, 1.0, 1.0),
  vec3(1.0, -1.0, -1.0),
  vec3(1.0, -1.0, 1.0),

  vec3(-1.0, -1.0, -1.0),
  vec3(1.0, -1.0, -1.0),
  vec3(1.0, 1.0, -1.0),

  vec3(-1.0, -1.0, -1.0),
  vec3(1.0, 1.0, -1.0),
  vec3(-1.0, 1.0, -1.0),

  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0, 1.0, 1.0),
  vec3(-1.0, -1.0, 1.0),

  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0, 1.0, -1.0),
  vec3(-1.0, 1.0, 1.0),

  vec3(1.0, 1.0, 1.0),
  vec3(-1.0, 1.0, 1.0),
  vec3(-1.0, 1.0, -1.0),

  vec3(1.0, 1.0, 1.0),
  vec3(-1.0, 1.0, -1.0),
  vec3(1.0, 1.0, -1.0),

  vec3(-1.0, -1.0, -1.0),
  vec3(1.0, -1.0, 1.0),
  vec3(1.0, -1.0, -1.0),

  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0, -1.0, 1.0),
  vec3(1.0, -1.0, 1.0)
);


void main() {
  vec3 pos = positions[gl_VertexIndex];
  rayOrigin = pos;
	gl_Position = ubo.data.worldToScreen * vec4(pos, 1.0);
}