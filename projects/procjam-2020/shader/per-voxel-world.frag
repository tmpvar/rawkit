#version 450

#include "../shared.h"

layout (location = 0) out vec4 outFragColor;

layout (std430, binding = 1) uniform UBO {
  world_ubo_t data;
} ubo;

in vec3 rayOrigin;

void main() {
  outFragColor = vec4(1.0, 0.0, 1.0, 1.0);
}