#version 450
#include "shared.h"
layout (location = 0) out vec4 color;
layout (std430, binding = 1) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 2) readonly buffer Bricks {
  Brick bricks[];
};

in vec3 rayOrigin;
in vec3 normal;
flat in vec3 eye;
flat in uint brick_id;


vec3 brick_dims = vec3(16.0);

#define BRICK_DIAMETER 4.0

float brick_march(in Brick brick, in vec3 rayOrigin, in vec3 rayDir, out vec3 normal, out float iterations) {
  rayOrigin -= rayDir * 3.0;
	vec3 pos = floor(rayOrigin);
	vec3 dir = sign(rayDir);
	vec3 sideDist = pos + 0.5 + dir * 0.5 - rayOrigin;
  vec3 invDir = 1.0 / rayDir;
	vec3 dt = sideDist * invDir;

  float max_iterations = length(brick_dims);
  for (iterations = 0; iterations < max_iterations; iterations++) {
    vec3 mm = step(vec3(dt.xyz), vec3(dt.yxy)) * step(vec3(dt.xyz), vec3(dt.zzx));
    sideDist = mm * dir;

    pos += sideDist;
    dt += sideDist * invDir;

    if (all(greaterThanEqual(pos, vec3(0.0))) && all(lessThan(pos, brick_dims))) {
      if (distance(floor(pos) + 0.5, vec3(8.0)) - 7.5 < 0.0) {
        normal = -sideDist;
        return 1.0;
      }
    }
  }

  return 0.0;
}

void main() {
  Brick brick = bricks[brick_id];
  vec3 pos = rayOrigin * brick_dims;
  vec3 dir = normalize(rayOrigin - eye);

  vec3 normal;
  float iterations;

  float hit = brick_march(
    brick,
    pos,
    dir,
    normal,
    iterations
  );

  color = smoothstep(
    vec4(.9),
    vec4(1.0),
    vec4(hit)
  );

  if (hit == 0.0) {
    discard;
  } else {
    color = vec4((normal + 1.0) * 0.5, 1.0);
  }
}
