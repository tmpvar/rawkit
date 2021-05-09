#version 450

#include "../shared.h"
layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 1) readonly buffer Positions {
  uvec4 positions[];
};

// a cube
const vec3 cube_vert_positions[36] = vec3[36](
  vec3(0.0, 0.0, 1.0),
  vec3(1.0, 1.0, 1.0),
  vec3(1.0, 0.0, 1.0),

  vec3(0.0, 0.0, 1.0),
  vec3(0.0, 1.0, 1.0),
  vec3(1.0, 1.0, 1.0),

  vec3(1.0, 1.0, 1.0),
  vec3(1.0, 1.0, 0.0),
  vec3(1.0, 0.0, 0.0),

  vec3(1.0, 1.0, 1.0),
  vec3(1.0, 0.0, 0.0),
  vec3(1.0, 0.0, 1.0),

  vec3(0.0, 0.0, 0.0),
  vec3(1.0, 0.0, 0.0),
  vec3(1.0, 1.0, 0.0),

  vec3(0.0, 0.0, 0.0),
  vec3(1.0, 1.0, 0.0),
  vec3(0.0, 1.0, 0.0),

  vec3(0.0, 0.0, 0.0),
  vec3(0.0, 1.0, 1.0),
  vec3(0.0, 0.0, 1.0),

  vec3(0.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 1.0, 1.0),

  vec3(1.0, 1.0, 1.0),
  vec3(0.0, 1.0, 1.0),
  vec3(0.0, 1.0, 0.0),

  vec3(1.0, 1.0, 1.0),
  vec3(0.0, 1.0, 0.0),
  vec3(1.0, 1.0, 0.0),

  vec3(0.0, 0.0, 0.0),
  vec3(1.0, 0.0, 1.0),
  vec3(1.0, 0.0, 0.0),

  vec3(0.0, 0.0, 0.0),
  vec3(0.0, 0.0, 1.0),
  vec3(1.0, 0.0, 1.0)
);

vec3 normals[6] = vec3[6](
  vec3(-1.0,  0.0,  0.0),
  vec3( 0.0, -1.0,  0.0),
  vec3( 0.0,  0.0, -1.0),
  vec3( 1.0,  0.0,  0.0),
  vec3( 0.0,  1.0,  0.0),
  vec3( 0.0,  0.0,  1.0)
);


out vec3 rayOrigin;
out vec3 normal;
out vec3 worldPos;
flat out vec3 eye;
flat out uint brick_id;
flat out vec3 brick_color;

vec3 hsl( in vec3 c ) {
  vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
  return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}


void main() {
  brick_id = gl_InstanceIndex;
  int face_idx = gl_VertexIndex / 3 / 2;
  normal = normals[face_idx];

  rayOrigin = cube_vert_positions[gl_VertexIndex];

  uvec4 pos = positions[gl_InstanceIndex];
  worldPos = vec3(pos.xyz);

  vec3 m = vec3(0.0);
  if (normal.y > 0.0) {
    m = vec3(1.0);
  } else if (normal.x != 0.0) {
    m = vec3(0.7);
  } else if (normal.z != 0.0) {
    m = vec3(-0.9);
  } else {
    m = vec3(0.2);
  }

  float l = length(vec2(127));
  float d = distance(worldPos.xz, vec2(127, 127));

  uint color = uint(pos.w);
  brick_color = pos.w != 0
    ? vec3(
      float((color) & 0xFF) / 255.0,
      float((color >> 8) & 0xFF) / 255.0,
      float((color >> 16) & 0xFF) / 255.0
    )
    : hsl(vec3(0.4 + d / l , 0.9, 0.3)) + m * 0.015;

  // uint brick_pos_bits = positions[gl_InstanceIndex];
  // worldPos = vec3(
  //   (brick_pos_bits) & 0xFF,
  //   (brick_pos_bits >> 8) & 0xFF,
  //   (brick_pos_bits >> 16) & 0xFF
  // );

  // vec3 brick_pos = positions[gl_InstanceIndex].xyz;

  eye = ubo.scene.eye.xyz - worldPos.xyz;


	gl_Position = ubo.scene.worldToScreen * vec4(
    rayOrigin + worldPos.xyz,
    1.0
  );
}