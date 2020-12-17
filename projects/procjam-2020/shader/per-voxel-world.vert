#version 450

#include "../shared.h"

layout (std430, binding = 1) uniform UBO {
  world_ubo_t data;
} ubo;

layout(std430, binding = 2) buffer VisibleVoxels {
	visible_voxel voxels[];
};

layout(binding = 3) uniform sampler3D world_texture;
layout(binding = 4) uniform sampler3D world_occlusion_texture;

out vec3 rayOrigin;
flat out vec3 voxelPos;
out vec4 color;

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

float t = ubo.data.time * 0.5;

void main() {
  vec3 dims = ubo.data.world_dims.xyz;
  vec3 pos = positions[gl_VertexIndex];
  vec3 voxel_pos = voxels[gl_InstanceIndex].pos.xyz;
  rayOrigin = (pos + voxel_pos) / dims;
  voxelPos = pos / dims;
  int face_idx = gl_VertexIndex / 3 / 2;
  vec3 normal = normals[face_idx];
  vec4 r = ubo.data.worldToScreen * vec4(rayOrigin, 1.0);
  color = vec4((pos + voxel_pos) / dims, 1.0);
  color = vec4(normal, 1.0);
  color = voxels[gl_InstanceIndex].face_color[face_idx];

  // color = vec4(pos, 1.0);
  gl_Position = r;
}