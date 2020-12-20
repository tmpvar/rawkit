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


vec3 brick_dims = vec3(4.0);

#define BRICK_DIAMETER 4.0



ray_hit_t march(scene_t scene, in vec3 pos, in vec3 dir) {
  float max_distance = 2.0;
  ray_hit_t ret;
  ret.color = vec4(0.0);
  ret.density = 0.0;
  ret.pos = pos;
  ret.pos -= dir * 0.1;

  vec3 mapPos = vec3(floor(pos));
  vec3 deltaDist = abs(vec3(length(dir)) / dir);
  vec3 rayStep = sign(dir);
  vec3 initial = pos;
  float dist = 0.0;
  vec3 sideDist = (sign(dir) * (mapPos - pos) + (sign(dir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  float hit = 0.0;
  vec3 prevPos = pos;
  float max_iterations = length(brick_dims) * 2.0;
  for (int iterations = 0; iterations < max_iterations; iterations++) {
    if (all(greaterThanEqual(mapPos, vec3(0))) && all(lessThan(mapPos, brick_dims))) {

      uint loc = uint(
        mapPos.x +
        mapPos.y * scene.grid_dims.x +
        mapPos.z * scene.grid_dims.x * scene.grid_dims.y
      );

      if (scene.brick.occlusion[loc] > 0) {
        break;
      }
    }
    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }

  // Note: this causes entire voxels to return the same coords
  ret.pos = floor(mapPos) + 0.5;
  // TODO: currently this only handles positive normals..
  ret.normal = mask;//(-mask*sign(dir)) * 0.5 + 0.5;

  return ret;
}

float march_voxviz(in Brick brick, in out vec3 pos, vec3 rayDir, out vec3 normal, out float iterations) {
  pos -= rayDir * 3.0;
  vec3 mapPos = vec3(floor(pos));
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 sideDist = (sign(rayDir) * (mapPos - pos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  vec3 offset = vec3(0.0);//brick.pos.xyz * brick_dims;

  float hit = 0.0;
  vec3 prevPos = pos;
  for (int iterations = 0; iterations < 16; iterations++) {
    ivec3 p = ivec3(mapPos - offset);
    if (all(greaterThanEqual(p, ivec3(0))) && all(lessThan(p, ivec3(4)))) {
      if (distance(mapPos, vec3(2.0)) - 1.5 < 0.0) {
        hit = 1.0;
        break;
      }

      // uint loc = uint(
      //   p.x +
      //   p.y * brick_dims.x +
      //   p.z * brick_dims.x * brick_dims.y
      // );

      // if (brick.occlusion[loc] > 0) {
      //   hit = 1.0;
      //   break;
      // }
    }
    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }

  pos = floor(mapPos) + 0.5;
  normal = mask;
  return hit;
}



void main() {
  Brick brick = bricks[brick_id];
  vec3 pos = rayOrigin * brick_dims;
  // vec3 eye = ubo.scene.eye.xyz * brick_dims;
  vec3 dir = normalize(rayOrigin - eye);

  vec3 normal;
  float iterations;

  float hit = march_voxviz(
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
    color = vec4(0.1);
    discard;
  } else {
    color = vec4(normal, 1.0);
  }
}
