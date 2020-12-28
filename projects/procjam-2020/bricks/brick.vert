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

out vec3 rayOrigin;
out vec3 normal;
flat out vec3 eye;
flat out uint brick_id;

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

vec3 normals[6] = vec3[6](
  vec3(0.0, 0.5, 0.5),
  vec3(0.5, 0.0, 0.5),
  vec3(0.5, 0.5, 0.0),
  vec3(1.0, 0.5, 0.5),
  vec3(0.5, 1.0, 0.5),
  vec3(0.5, 0.5, 1.0)
);

void main() {
  uint64_t occlusion[4];
  Brick brick = bricks[gl_InstanceIndex];
  brick_id = gl_InstanceIndex;

  rayOrigin = positions[gl_VertexIndex];
  eye = ubo.scene.eye.xyz - brick.pos.xyz;

  int face_idx = gl_VertexIndex / 3 / 2;
  normal = normals[face_idx];

	gl_Position = ubo.scene.worldToScreen * vec4(
    rayOrigin + brick.pos.xyz,
    1.0
  );
}