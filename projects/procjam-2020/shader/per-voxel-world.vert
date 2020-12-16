#version 450

#include "../shared.h"

layout (std430, binding = 1) uniform UBO {
  world_ubo_t data;
} ubo;

layout(std430, binding = 2) buffer VisibleVoxels {
	visible_voxel voxels[];
};

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

void main() {
  vec3 dims = ubo.data.world_dims.xyz;
  vec3 pos = positions[gl_VertexIndex];
  vec3 voxel_pos = voxels[gl_InstanceIndex].pos.xyz;
  rayOrigin = (pos + voxel_pos);// / dims;
  voxelPos = (voxel_pos + 0.5);// / dims;
  vec4 r = ubo.data.worldToScreen * vec4(rayOrigin / dims, 1.0);
  color = vec4((pos + voxel_pos) / dims, 1.0);
  color = vec4(normals[gl_VertexIndex / 3 / 2], 1.0);
  // color = vec4(pos, 1.0);

  gl_Position = r;
}