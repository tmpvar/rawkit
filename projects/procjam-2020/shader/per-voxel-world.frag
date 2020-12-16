#version 450

#include "../shared.h"

layout (location = 0) out vec4 outFragColor;

layout (std430, binding = 1) uniform UBO {
  world_ubo_t data;
} ubo;

in vec3 rayOrigin;
flat in vec3 voxelPos;
in vec4 color;

void main() {
  outFragColor = vec4(1.0, 0.0, 1.0, 1.0);
  outFragColor = vec4(voxelPos, 1.0);
  outFragColor = vec4(1.0 - voxelPos.z / 100.0);
  outFragColor = color;
  // outFragColor = vec4(gl_FragCoord.z, color.r, 0.0, 1.0);
}