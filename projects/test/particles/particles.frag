#version 450

#include "shared.h"

layout (location = 0) out vec4 outColor;

layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

in vec2 uv;

void main() {
  float c = smoothstep(
    1.0,
    0.5,
    length(uv) - 0.1
  );
  outColor = vec4(1.0, 1.0, 1.0, c);
}