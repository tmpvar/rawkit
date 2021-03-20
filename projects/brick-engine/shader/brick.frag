#version 450
#include "../shared.h"
#include "uberprim.glsl"

layout (location = 0) out vec4 color;
layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 1) readonly buffer Bricks {
  Brick bricks[];
};

in vec3 rayOrigin;
flat in vec3 eye;
flat in uint brick_id;
flat in float mip;

vec3 brick_dims = vec3(64.0);

// taken from: https://www.shadertoy.com/view/WslGRN
vec3 heat(float percent) {
  float level = percent*3.14159265/2.;
  return vec3(
    sin(level),
    sin(level*2.),
    cos(level)
  );
}

float brick_march(in Brick brick, in vec3 rayOrigin, in vec3 rayDir, out vec3 normal, out float iterations) {
  rayOrigin -= rayDir * 1.0;
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

      #if 0
      if (distance(floor(pos) + 0.5, brick_dims * 0.5) - brick_dims.x * 0.5 < 0.0) {
        normal = -sideDist;
        return 1.0;
      }
      #else

        // float d = doModel((floor(pos) - 32.0), ubo.scene.time);
        float d = sdUberprim(pos / 32.0 - 0.5, vec4(1.0), vec3(1.0, 1.0, 0.0));
        if (d < 0.0){
          normal = -sideDist;
          return 1.0;
        }
      #endif
    }
  }

  return 0.0;
}

float mincomp(vec3 v) {
  return min(v.x, min(v.y, v.z));
}

float brick_march_mip_aware(in Brick brick, in vec3 rayOrigin, in vec3 rayDir, in float mip, out vec3 normal, out float iterations) {
	vec3 dir = normalize(rayDir);
  vec3 invDir = 1.0 / dir;
	vec3 sideDist = floor(rayOrigin) + 0.5 + dir * 0.5 - rayOrigin;

  float mipSize = (float(1<<(clamp(uint(mip), 0, 6))));
  float invMipSize = 1.0 / mipSize;
  float stepSize = 1.0 / (brick_dims.x / mipSize);

  float max_iterations = length(brick_dims);
  float t = 0.0;

  vec3 center = brick_dims * 0.5;
  normal = vec3(1.0, 0.0, 0.0);
  for (iterations = 0; iterations < max_iterations; iterations++) {
    vec3 p = rayOrigin + dir * t;

    #if 0
      float d = distance(p, vec3(0.5)) - 0.5;
      if (d < stepSize) {
        normal = p;// * 0.01;//-sideDist;
        return 1.0;
      }
    #else

      float d = doModel((p - 0.5) * 2.0, ubo.scene.time.x * 2.1);
      // float d = sdUberprim((p - 0.5) * 2.0, vec4(1.0), vec3(1.0, 1.0, 0.0));
      if (d < stepSize){
        normal = p;
        return 1.0;
      }
    #endif



    // float dt = max(mincomp(deltas), 0.0001) * mipSize;
    // vec3 deltas = (step(0.0, dir) - fract(p * invMipSize)) * invDir;
    t += stepSize;
  }

  return 0.0;
}

void main() {

  Brick brick = bricks[brick_id];

  vec3 pos = rayOrigin * brick_dims;
  vec3 dir = normalize(rayOrigin - eye);
  color = vec4(rayOrigin, 1.0);

  vec3 normal;
  float iterations;

  #if 0
    float hit = brick_march(
      brick,
      pos,
      dir,
      normal,
      iterations
    );
  #else
    float hit = brick_march_mip_aware(
      brick,
      rayOrigin,
      dir,
      6.0 - clamp(floor(mip), 0.0, 6.0),
      normal,
      iterations
    );
  #endif

  color = smoothstep(
    vec4(.9),
    vec4(1.0),
    vec4(hit)
  );

  if (hit == 0.0) {
    discard;
  } else {
    color = (vec4((normal + 1.0) * 0.5, 1.0) + vec4(heat(floor(mip)/8.0), 1.0)) * 0.5;
    // color = vec4(heat(floor(mip)/8.0), 1.0);
    color = vec4((normal + 1.0) * 0.5, 1.0);
    // color = vec4(vec3(dot(normal, vec3(.5, .5, 0.0))) + 0.5, 1.0);
  }
}
