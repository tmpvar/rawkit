#version 450
  #extension GL_ARB_gpu_shader_int64 : require
  #extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../shared.h"
#include "quadric-proj.glsl"

layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 1) readonly buffer Bricks {
  Brick bricks[];
};

layout(std430, binding = 2) buffer Index {
  uint visibility[];
};

out vec3 rayOrigin;
flat out vec3 eye;
flat out uint brick_id;
flat out float mip;


void main() {
  uint idx = gl_VertexIndex;

  rayOrigin = vec3(
    (idx >> 0) & 1,
    (idx >> 1) & 1,
    (idx >> 2) & 1
  );

  uint instance_id = (idx >> 3);

  uint brick_id = visibility[instance_id];
  Brick brick = bricks[brick_id];
  vec3 pos = brick.pos.xyz;

  mip = brick_mip(
    pos,
    1.0,
    ubo.scene.worldToScreen,
    ubo.scene.screen_dims.xy
  );

  eye = ubo.scene.eye.xyz - pos;

  if (eye.x > 0.0) rayOrigin.x = 1.0 - rayOrigin.x;
  if (eye.y > 0.0) rayOrigin.y = 1.0 - rayOrigin.y;
  if (eye.z > 0.0) rayOrigin.z = 1.0 - rayOrigin.z;

	gl_Position = ubo.scene.worldToScreen * vec4(
    rayOrigin + pos,
    1.0
  );

}