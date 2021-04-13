#version 450

#include "shared.h"

layout (location = 0) out vec4 outColor;

layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

in vec2 uv;

void main() {
  // // TODO: circle
  // if (length(uv) > RADIUS * 2.0 - 6) {
  //   discard;
  // } else {
  //   outColor = vec4(1.0);
  // }

  float c = smoothstep(
    1.0,
    0.85,
    length(uv)
  );
  outColor = vec4(1.0, 1.0, 1.0, c);
//1.0 - smoothstep( 1.0 - d, 1.0 + d, r )
}