#version 450

#include "../shared.h"
#include "ray-aabb.glsl"

layout (location = 0) out vec4 outFragColor;

layout (std430, binding = 1) uniform UBO {
  world_ubo_t data;
} ubo;

layout(binding = 2) uniform sampler3D world_texture;
layout(binding = 3) uniform sampler3D world_occlusion_texture;
layout(binding = 4) uniform sampler2D blue_noise;

in vec3 rayOrigin;
light_t sun;

float march_light(in out vec3 pos, vec3 rayDir, out vec3 normal, out float iterations, out vec4 color) {
  vec3 ratio = 1.0 / ubo.data.world_dims.xyz;
  vec3 initial = pos;
  float max_iterations = length(ubo.data.world_dims);
  float hit = 0.0;
  color = vec4(0.0);
  for (iterations = 0.0; iterations < max_iterations; iterations++) {
    vec3 uvw = pos / ubo.data.world_dims.xyz;

    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0)))) {
      return 0.0;
    }

    if (texture(world_occlusion_texture, uvw).r > 0.0) {
      color = texture(world_texture, uvw);
      return 1.0;
    }

    pos = initial + iterations * rayDir * 2.0;
  }

  return 0.0;
}

float march_low_density(in out vec3 pos, vec3 rayDir, out float iterations, out vec4 color) {
  vec3 ratio = 1.0 / ubo.data.world_dims.xyz;
  vec3 initial = pos;
  float max_iterations = length(ubo.data.world_dims)* 0.5;
  float hit = 0.0;
  color = vec4(0.0);
  for (iterations = 0.0; iterations < max_iterations; iterations++) {
    vec3 uvw = pos / ubo.data.world_dims.xyz;

    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0)))) {
      return hit;
    }

    float density = texture(world_occlusion_texture, uvw).r;
    if (density > 0.0 && density < 1.0) {
      color += texture(world_texture, uvw) * density;
    }
    hit += density;


    if (hit > 1.0) {
      break;
    }

    pos = initial + iterations * rayDir * 2.0;
  }

  return hit;
}


float march_brick(in out vec3 pos, vec3 rayDir, out vec3 normal, out float iterations, out vec4 color) {
  vec3 del = fract(pos) - rayDir;
  pos -= rayDir * 0.0001;//step(del.xyz, del.yzx) * step(del.xyz, del.zxy) * 0.1;
  vec3 mapPos = vec3(floor(pos));
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 initial = pos;
  float dist = 0.0;
  vec3 sideDist = (sign(rayDir) * (mapPos - pos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  float hit = 0.0;
  vec3 prevPos = pos;
  vec3 ratio = 1.0 / ubo.data.world_dims.xyz;
  float max_iterations = length(ubo.data.world_dims.xyz) * 2.0;
  color = vec4(0.0);
  for (int iterations = 0; iterations < max_iterations; iterations++) {
    if (all(greaterThanEqual(mapPos, vec3(0))) && all(lessThan(mapPos, ubo.data.world_dims.xyz))) {
      vec3 p = mapPos * ratio;
      float density = texture(world_occlusion_texture, p).r;

      hit += density;
      color += texture(world_texture, p);

      // if (density > 0.0) {
      //   if (density < 0.5) {
      //     vec3 low_density_pos = floor(mapPos) + 0.5;
      //     float low_density_iterations = 0.0f;
      //     vec4 low_density_color;
      //     float low_density = march_low_density(
      //       low_density_pos,
      //       normalize(sun.pos - low_density_pos),
      //       low_density_iterations,
      //       low_density_color
      //     );

      //     color += texture(world_texture, p);
      //     hit += density * low_density;

      //     // color += low_density_color * 0.001;
      //   } else {
      //    color += texture(world_texture, p);
      //     hit += density;
      //   }
      // }

      if (hit >= 1.0) {
        break;
      }
    }
    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }

  // Note: this causes entire voxels to return the same coords
  pos = floor(mapPos) + 0.5;

  normal = (-mask*sign(rayDir)) * 0.5 + 0.5;
  return hit;
}

float march(in out vec3 pos, vec3 rayDir, out vec3 normal, out float iterations, out vec4 color) {
  vec3 ratio = 1.0 / ubo.data.world_dims.xyz;
  pos -= rayDir * 0.0001;
  vec3 mapPos = floor(pos);

  vec3 prevPos = pos * ratio;//mapPos * ratio;
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 sideDist = (rayStep * (mapPos - pos) + (rayStep * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
  normal = mask;
  float hit = 0.0;
  float max_iterations = length(ubo.data.world_dims);
  color = vec4(0.0, 0.0, 0.0, 1.0);
  for (iterations = 0.0; iterations < max_iterations; iterations++) {
    vec3 p = mapPos * ratio;
    if (any(lessThanEqual(p, vec3(0.0))) || any(greaterThanEqual(p, vec3(1.0)))) {
      hit = 0.0;
      break;
    }

    float density = texture(world_occlusion_texture, p).r;
    if (density > 0.0) {
      color = texture(world_texture, p);
      return 1.0;
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    normal = mask;
    sideDist += mask * deltaDist;
    prevPos = p;
    mapPos += mask * rayStep;
    pos += mask * rayStep;

  }

  return hit;
}

float t = ubo.data.time * 0.5;


void main() {
  sun.pos = (vec3(
    sin(t) * 2.0,
    2.0,
    cos(t) * 2.0
  ) * 0.5 + 0.5) * ubo.data.world_dims.xyz;

  sun.color = vec3(1.0, 0.7, 0.1);

  vec3 dir = normalize(rayOrigin - ubo.data.eye.xyz);

  // from -1..1 to 0..1
  vec3 uv = rayOrigin * 0.5 + 0.5;

  vec3 pos = uv * ubo.data.world_dims.xyz;
  vec3 center;
  vec3 worldNormal;
  float iterations = 0.0f;
  float colorIterations = 0.0f;
  vec4 outColor;
  float hit = march_brick(
    pos,
    dir,
    worldNormal,
    colorIterations,
    outColor
  );

  float density = texture(world_occlusion_texture, pos / ubo.data.world_dims.xyz).r;

  // // 0..dims to -1..1
  // // vec3 worldPos = (pos / ubo.data.world_dims.xyz) * 2.0  - 1.0;

  // vec3 ratio = 1.0 / ubo.data.world_dims.xyz;
  float hardOcclusion = 0.0;
  vec3 normal;
  {
    vec4 lightColor;
    float lightIterations = 0.0f;
    float sunOcclusion = 0.0;
    vec3 lightDir = normalize(sun.pos - pos);

    vec3 lightPos = (
      density < 1.0 ? pos : pos + worldNormal * 0.55
      // + dot(dir, worldNormal)
      // // + dir * 0.15
      // + dir * 0.5
      // + worldNormal * 0.5// * 1.0 - max(0.0, dot(dir, worldNormal))
    );
    vec4 c = vec4(0.0);
    hardOcclusion = march_brick(
      lightPos,
      lightDir,
      normal,
      lightIterations,
      c
    );
  }

  vec4 lightColor;
  float lightIterations = 0.0f;
  float sunOcclusion = 1.0;
  // const float steps = 5.0;

  // for (float i=0.0; i<steps; i++) {
  //   vec3 target = sun.pos + (texture(blue_noise, pos.xz * i).xyz * 2.0 - 1.0) * 2.0;
  //   vec3 lightDir = normalize(target - pos );

  //   vec3 lightPos = (
  //     pos + lightDir + worldNormal
  //   );
  //   vec4 c = vec4(0.0);

  //   float occ = march_brick(
  //     lightPos,
  //     lightDir,
  //     normal,
  //     lightIterations,
  //     c
  //   );

  //   sunOcclusion += occ / steps;
  //   lightColor += (c / steps) * occ;
  //   // lightColor = c * occ;
  // }

  if (hit == 0.0) {
    outFragColor = vec4(0.0);
    return;
  }
  // outFragColor = outColor * hit;
  // outFragColor -= (hardOcclusion + vec4(sunOcclusion) * 0.5) * 0.5;
  // outFragColor = vec4(outColor.rgb * max(1.0-hardOcclusion, 0.4) * sun.color, 1.0) * hit;
  // outFragColor = vec4(worldNormal * hit, 1.0);


  // pos is the voxel center
  // outFragColor *= 1.0 - (dot(normalize(sun.pos - pos), -worldNormal));
  outFragColor = (
    outColor / hit
    - hardOcclusion
  );


  outFragColor *= 1.0 - (dot(normalize(sun.pos - pos), -worldNormal)) * 0.7;

  // outFragColor = outColor + lightColor * 0.25;
  // vec4 noise = texture(blue_noise, pos.xz * 100.0);
  // outFragColor = (outColor * hit + vec4(hit * normal * 0.05, 1.0)) * dot(sun_dir, dir) * 3.0;
  // outFragColor = outColor + (1.0-dot(hit * normal, dir)) * 0.09;
  // outFragColor = outColor + (dot(hit * normal, dir)) * 0.2 * dot(sun_dir, dir);
  // if (hit > 0.0) {
  //   outFragColor = vec4(normal, 1.0);
  //   // + lightColor * 0.001) - sunOcclusion;
  //   // // outFragColor = vec4((colorIterations + lightIterations) / 256);
  //   // // (outColor + lightColor* 0.25) / 2.0 - (sunOcclusion * 0.5);//vec4(1.0 - sunOcclusion);
  //   // outFragColor = lightColor;
  //   // outFragColor = outColor - sunOcclusion;
    // outFragColor *= (0.1 + dot(normalize(sun_pos - pos), normal));
  // } else {
  //   outFragColor = vec4(0.0);
  // }
  // outFragColor =;
  // outFragColor = outColor;
  // outFragColor = vec4(normal, 1.0);
  // outFragColor = (outColor + lightColor) / 2.0 * 1.0 - sunOcclusion;


}