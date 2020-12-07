#version 450
#define ITERATIONS 256

#include "../shared.h"

layout (location = 0) out vec4 outFragColor;

layout (std430, binding = 1) uniform UBO {
  world_ubo_t data;
} ubo;

layout(binding = 2) uniform sampler3D world_texture;

in vec3 rayOrigin;

float march(in out vec3 pos, vec3 rayDir, out vec3 center, out vec3 normal, out float iterations, out vec4 color) {
  vec3 ratio = 1.0 / ubo.data.world_dims.xyz;

  vec3 mapPos = vec3(floor(pos));
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 sideDist = (rayStep * (mapPos - pos) + (rayStep * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  float hit = 0.0;

  for (iterations = 0.0; iterations < ITERATIONS; iterations++) {
    vec4 c = texture(world_texture, floor(mapPos) * ratio);
    if (any(greaterThan(c.rgb, vec3(0.0)))) {
      color = c;
      hit = 1.0;
      break;
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }

  // pos = floor(mapPos) + 0.5;
  normal = mask;
  return hit;
}

void main() {
  vec3 dir = normalize(rayOrigin - ubo.data.eye.xyz);

  // from -1..1 to 0..1
  vec3 uv = rayOrigin * 0.5 + 0.5;

  vec3 pos = uv * ubo.data.world_dims.xyz;
  vec3 center;
  vec3 normal;
  float iterations = 0.0f;
  vec4 outColor;
  float hit = march(
    pos,
    dir,
    center,
    normal,
    iterations,
    outColor
  );

  vec4 color = texture(world_texture, uv);
  if (hit == 1.0) {
    outFragColor = vec4(1.0);
    outFragColor = vec4(normal, 1.0);
    // outFragColor = vec4(pos / vec3(128.0), 1.0);
  } else {
    outFragColor = vec4(0.0);
  }
}