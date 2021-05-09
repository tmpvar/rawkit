#version 450

#include "../shared.h"
layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 1) readonly buffer Positions {
  uint positions[];
};

// a cube
const vec3 cube_vert_positions[36] = vec3[36](
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

out vec3 rayOrigin;
out vec3 normal;
flat out vec3 eye;
flat out uint brick_id;

void main() {
  brick_id = gl_InstanceIndex;

  rayOrigin = cube_vert_positions[gl_VertexIndex];
  uint brick_pos_bits = positions[gl_InstanceIndex];
  vec3 brick_pos = vec3(
    (brick_pos_bits) & 0xFF,
    (brick_pos_bits >> 8) & 0xFF,
    (brick_pos_bits >> 16) & 0xFF
  );

  // vec3 brick_pos = positions[gl_InstanceIndex].xyz;

  eye = ubo.scene.eye.xyz - brick_pos.xyz;

  int face_idx = gl_VertexIndex / 3 / 2;
  normal = normals[face_idx];

	gl_Position = ubo.scene.worldToScreen * vec4(
    rayOrigin + brick_pos.xyz,
    1.0
  );
}