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
const vec2 positions[6] = vec2[6](
  vec2(-0.5, -0.5),
  vec2( 0.5,  0.5),
  vec2( 0.5, -0.5),

  vec2(-0.5, -0.5),
  vec2(-0.5,  0.5),
  vec2( 0.5,  0.5)
);

float t = ubo.data.time * 0.5;

//Fast Quadric Proj: "GPU-Based Ray-Casting of Quadratic Surfaces" http://dl.acm.org/citation.cfm?id=2386396
void quadricProj(
  in vec3 osPosition,
  in float voxelSize,
  in mat4 objectToScreenMatrix,
  in vec2 halfScreenSize,
  inout vec4 position,
  inout float pointSize
) {
  const vec4 quadricMat = vec4(1.0, 1.0, 1.0, -1.0);
  float sphereRadius = voxelSize * 1.732051;
  vec4 sphereCenter = vec4(osPosition.xyz, 1.0);
  mat4 modelViewProj = transpose(objectToScreenMatrix);
  mat3x4 matT = mat3x4(
     mat3(modelViewProj[0].xyz, modelViewProj[1].xyz, modelViewProj[3].xyz) * sphereRadius
  );
  matT[0].w = dot(sphereCenter, modelViewProj[0]);
  matT[1].w = dot(sphereCenter, modelViewProj[1]);
  matT[2].w = dot(sphereCenter, modelViewProj[3]);

  mat3x4 matD = mat3x4(matT[0] * quadricMat, matT[1] * quadricMat, matT[2] * quadricMat);
  vec4 eqCoefs = vec4(
    dot(matD[0], matT[2]),
    dot(matD[1], matT[2]),
    dot(matD[0], matT[0]),
    dot(matD[1], matT[1])
  ) / dot(matD[2], matT[2]);

  vec4 outPosition = vec4(eqCoefs.x, eqCoefs.y, 0.0, 1.0);
  vec2 AABB = sqrt(eqCoefs.xy*eqCoefs.xy - eqCoefs.zw);
  AABB *= halfScreenSize * 2.0f;

  position.xy = outPosition.xy * position.w;
  pointSize = max(AABB.x, AABB.y);
}



void main() {

  const visible_voxel voxel = voxels[gl_InstanceIndex];
  vec2 pos = positions[gl_VertexIndex];
  vec3 dims = ubo.data.world_dims.xyz;
  vec4 voxel_pos = vec4(voxel.pos.xyz / dims, 1.0);
  int face_idx = gl_VertexIndex / 3 / 2;

  vec4 p = vec4(1.0);
  p.w = distance(voxel.pos.xyz / dims, ubo.data.eye.xyz / dims);
  float pointSize = 1.0;
  vec2 screen = ubo.data.screen_dims.xy;
  quadricProj(
    vec3(voxel.pos.xyz),
    1.0,
    ubo.data.worldToScreen,
    ubo.data.screen_dims.xy * 0.5,
    p,
    pointSize
  );

  gl_Position = (ubo.data.worldToScreen * voxel_pos) + vec4(pos * 0.01, 0.0, 1.0);

  color = vec4(pos, 0.0, 1.0);
  color = vec4(voxel_pos.xyz, 1.0);
  color = voxel.face_color[face_idx];
}