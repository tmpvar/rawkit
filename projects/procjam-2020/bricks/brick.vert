#version 450
  #extension GL_ARB_gpu_shader_int64 : require
  #extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "shared.h"

layout (std430, binding = 1) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 2) readonly buffer Bricks {
  Brick bricks[];
};

out vec4 v_color;

// a cube
const vec3 positions[36] = vec3[36](
  vec3(0.0, 0.0, 1.0),
  vec3(1.0, 1.0, 1.0),
  vec3(1.0, 0.0, 1.0),

  vec3(0.0, 0.0, 1.0),
  vec3(0.0, 1.0, 1.0),
  vec3(1.0, 1.0, 1.0),

  vec3(1.0, 1.0, 1.0),
  vec3(1.0, 1.0, 0.0),
  vec3(1.0, 0.0, 0.0),

  vec3(1.0, 1.0, 1.0),
  vec3(1.0, 0.0, 0.0),
  vec3(1.0, 0.0, 1.0),

  vec3(0.0, 0.0, 0.0),
  vec3(1.0, 0.0, 0.0),
  vec3(1.0, 1.0, 0.0),

  vec3(0.0, 0.0, 0.0),
  vec3(1.0, 1.0, 0.0),
  vec3(0.0, 1.0, 0.0),

  vec3(0.0, 0.0, 0.0),
  vec3(0.0, 1.0, 1.0),
  vec3(0.0, 0.0, 1.0),

  vec3(0.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 1.0, 1.0),

  vec3(1.0, 1.0, 1.0),
  vec3(0.0, 1.0, 1.0),
  vec3(0.0, 1.0, 0.0),

  vec3(1.0, 1.0, 1.0),
  vec3(0.0, 1.0, 0.0),
  vec3(1.0, 1.0, 0.0),

  vec3(0.0, 0.0, 0.0),
  vec3(1.0, 0.0, 1.0),
  vec3(1.0, 0.0, 0.0),

  vec3(0.0, 0.0, 0.0),
  vec3(0.0, 0.0, 1.0),
  vec3(1.0, 0.0, 1.0)
);

void main() {
  uint64_t occlusion[4];
  Brick brick = bricks[gl_InstanceIndex];
  vec3 pos = positions[gl_VertexIndex] + brick.pos.xyz;
  v_color = vec4(pos.x / 16.0, pos.y / 16.0, 0.0, 1.0);
	gl_Position = ubo.scene.worldToScreen * vec4(pos, 1.0);
}