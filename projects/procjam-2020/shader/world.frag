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

struct ray_hit_t {
  vec4 color;
  vec3 pos;
  vec3 normal;
  float density;
  float iterations;
};

struct scene_t {
  vec3 grid_dims;
  vec3 inv_grid_dims;
};

ray_hit_t march_light(in out vec3 pos, vec3 dir) {

  ray_hit_t ret;
  ret.color = vec4(0.0);
  ret.density = 0.0;
  ret.pos = pos;
  ret.pos -= dir * 0.1;

  vec3 ratio = 1.0 / ubo.data.world_dims.xyz;
  vec3 initial = pos;
  float max_iterations = length(ubo.data.world_dims);

  for (float iterations = 0.0; iterations < max_iterations; iterations++) {
    vec3 uvw = pos / ubo.data.world_dims.xyz;

    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0)))) {
      break;
    }

    ret.density = texture(world_occlusion_texture, uvw).r;

    if (ret.density > 0.0) {
      ret.color = texture(world_texture, uvw);
      break;
    }

    pos = initial + iterations * dir * 2.0;
  }

  return ret;
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
  vec4 prevSample = vec4(0.0);
  for (int iterations = 0; iterations < max_iterations; iterations++) {
    if (all(greaterThanEqual(mapPos, vec3(0))) && all(lessThan(mapPos, ubo.data.world_dims.xyz))) {
      vec3 p = mapPos * ratio;
      float density = texture(world_occlusion_texture, p).r;
      vec4 s = texture(world_texture, p);

      if (density >= 1.0) {
        color.rgb += texture(world_texture, p).rgb;

        if (prevSample.a == 0.0) {
          prevSample.a = s.a;
        }

        if (hit == 0.0) {
          hit = 1.0;
        }

        break;
      }

      hit += density;

      color += s * density;
      color.a = s.a;
      prevSample = s;
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

ray_hit_t march_first_contact(scene_t scene, in vec3 pos, in vec3 dir, float max_acc_density) {
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
  vec3 ratio = 1.0 / ubo.data.world_dims.xyz;
  float max_iterations = length(ubo.data.world_dims.xyz) * 2.0;
  for (int iterations = 0; iterations < max_iterations; iterations++) {
    if (all(greaterThanEqual(mapPos, vec3(0))) && all(lessThan(mapPos, ubo.data.world_dims.xyz))) {
      vec3 p = mapPos * ratio;

      ret.density += texture(world_occlusion_texture, p).r;
      if (ret.density > max_acc_density) {
        ret.color = texture(world_texture, p);
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

float t = ubo.data.time * 0.5;


void main() {
  outFragColor = vec4(0.0);
  scene_t scene;
  scene.grid_dims = ubo.data.world_dims.xyz;
  scene.inv_grid_dims = 1.0 / scene.grid_dims;

  vec3 world_dims = ubo.data.world_dims.xyz;

  sun.pos = (vec3(
    sin(t) * 2.0,
    2.0,
    cos(t) * 2.0
  )) * world_dims;

  sun.color = vec3(1.0, 0.7, 0.1);



  // from -1..1 to 0..1 to grid spce
  vec3 pos = (rayOrigin) * world_dims;
  vec3 eye = (ubo.data.eye.xyz) * world_dims;
  vec3 dir = normalize(rayOrigin - ubo.data.eye.xyz);
  ray_hit_t hit = march_first_contact(scene, pos, dir, 0.0);


  // reflect the ray
  if (hit.color.a == WATER_ALPHA) {
    vec3 p = hit.pos * scene.inv_grid_dims;
    vec3 reflect_dir = normalize(reflect(dir, hit.normal));//vec3(0.0, 1.0, 0.0)));
    vec3 reflect_pos = hit.pos + hit.normal;//vec3(0.0, 0.51, 0.0);// + reflect_dir * vec3(0.0, 2.0, 0.0);

    ray_hit_t reflect_hit = march_first_contact(scene, reflect_pos, reflect_dir, 1.0);
    // TODO: this starts marching at the edge again, we should be able to continue but the raymarch
    //       function only returns a grid oriented position.
    ray_hit_t floor_hit = march_first_contact(
      scene,
      pos,
      dir,
      32.0
    );

    floor_hit.density /= 32.0;

    ray_hit_t sun_hit = march_first_contact(
      scene,
      hit.pos + vec3(0.0, 1.0, 0.0),
      normalize(sun.pos - pos),
      1.0
    );

    vec3 floor_sun_dir = normalize(sun.pos - floor_hit.pos);
    ray_hit_t floor_sun_hit = march_first_contact(
      scene,
      floor_hit.pos + vec3(0.0, 1.0, 0.0) + floor_sun_dir * 2.0,
      floor_sun_dir,
      100.0
    );
    floor_sun_hit.density /= 100.0;

    outFragColor = floor_hit.color * floor_hit.density;
    outFragColor += reflect_hit.color * reflect_hit.density * 0.1;

    outFragColor *= (1.0 - sun_hit.density * 0.1);
    outFragColor *= (1.0 - floor_sun_hit.density * 0.9);
    float m = 32;
    float underwater_dist = 1.0 - (max(
      m,
      distance(hit.pos, floor_hit.pos)
    ) / m);


    outFragColor *= (0.4 + vec4(
      underwater_dist,
      underwater_dist * 0.3,
      0,
      1.0
    ));

    if (floor_hit.density < 1.0) {
      outFragColor += vec4(
        0.0,
        1.0 - underwater_dist,
        1.0 - underwater_dist,
        1.0
      ) * 0.006;
    }

    // outFragColor = vec4(reflect_hit.density * reflect_hit.color.rgb, 1.0);
    // outFragColor = vec4(hit.color.rgb * reflect_hit.color.rgb, 1.0);
    // outFragColor += reflect_hit.density * dot(reflect_dir, reflect_hit.normal) * 0.1;
    //  outFragColor = vec4(reflect_hit.color.rgb, 1.0);
    //  outFragColor = vec4(hit.normal.b);


    // outFragColor = vec4(floor_sun_dir, 1.0);
  } else if (hit.density > 0.0) {

    vec3 n = -(hit.normal*sign(dir)) * 0.5 + 0.5;

    ray_hit_t sun_hit = march_first_contact(
      scene,
      hit.pos + n * 0.55,
      normalize(sun.pos - pos),
      1.0
    );

    outFragColor = vec4(hit.color.rgb * hit.density, 1.0);
    outFragColor += hit.density * dot(dir, hit.normal) * 0.25;
    outFragColor -= sun_hit.density * 0.1;

    outFragColor = (
      hit.color / hit.density
      - sun_hit.density * 0.25
    );
    // outFragColor = vec4(n, 1.0);
  }

  // outFragColor = vec4(reflect(dir, hit.normal), 1.0);
  #if 0
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
  vec4 lightColor;
  vec3 normal;
  {
    float lightIterations = 0.0f;
    float sunOcclusion = 0.0;
    vec3 lightDir = normalize(sun.pos - pos);

    vec3 lightPos = (
      //density < 1.0 ? pos : pos + worldNormal * 0.55
      pos + worldNormal * 0.55
      // + dot(dir, worldNormal)
      // // + dir * 0.15
      // + dir * 0.5
      // + worldNormal * 0.5// * 1.0 - max(0.0, dot(dir, worldNormal))
    );

    hardOcclusion = march_light(
      lightPos,
      lightDir,
      normal,
      lightIterations,
      lightColor
    );
  }

  float lightIterations = 0.0f;
  float sunOcclusion = 1.0;

  if (hit == 0.0) {
    outFragColor = vec4(0.0);
    return;
  }
  // outFragColor = outColor * hit;
  // outFragColor -= (hardOcclusion + vec4(sunOcclusion) * 0.5) * 0.5;
  // outFragColor = vec4(outColor.rgb * max(1.0-hardOcclusion, 0.4) * sun.color, 1.0) * hit;
  // outFragColor = vec4(worldNormal * hit, 1.0);


  // pos is the voxel center



  // outFragColor *= 1.0 - (dot(normalize(sun.pos - pos), -worldNormal)) * 0.9;
  if (outColor.a == WATER_ALPHA) {
    outFragColor = (
      outColor / hit * 0.99// / hardOcclusion * 0.5
    ) * 0.6;

    outFragColor -=  hardOcclusion * 0.155;
    // outFragColor *= (1.0 - dot(normalize(sun.pos - pos), -worldNormal)) * (0.99 - hardOcclusion);
    // outFragColor = lightColor;
    // outFragColor = vec4(hit);
    // outFragColor *= dot(normalize(sun.pos - pos), worldNormal) * 0.85;
    // outFragColor -= 0.1;
    //outFragColor /= hit;
    // outFragColor = vec4(1.0, 0.0, 1.0, 1.0);
    outFragColor *= (1.0 - dot(normalize(sun.pos - pos), -worldNormal)) * 0.55;
  } else {
    outFragColor = (
      outColor / hit
      - hardOcclusion * 0.25
    );

    outFragColor *= (1.0 - dot(normalize(sun.pos - pos), -worldNormal)) * 0.55;
  }


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

  #endif
}