#version 450

#include "shared.h"

layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 1) readonly buffer Particles {
  vec2 particles[];
};

out vec2 uv;

void main() {
  uint idx = gl_VertexIndex;
  uv = vec2(
    ((idx >> 0) & 1) == 1 ? 1.0 : -1.0,
    ((idx >> 1) & 1) == 1 ? 1.0 : -1.0
  );

  uint particle_id = idx >> 2;
  vec2 pos = 2.0 * particles[particle_id] - ubo.scene.dims + uv * RADIUS * 2.0;

	gl_Position = vec4(
    pos / ubo.scene.dims * vec2(1.0, -1.0),
    0.0,
    1.0
  );
}