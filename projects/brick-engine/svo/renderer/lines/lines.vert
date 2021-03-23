#version 450
#include "../../svo.h"

layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 1) readonly buffer NodeTree {
  InnerNode nodes[];
};

layout(std430, binding = 2) readonly buffer NodePositions {
  vec4 node_positions[];
};

out vec3 cellColor;

vec3 hsl( in vec3 c ) {
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

// a cube made of lines
const vec3 positions[24] = vec3[24](
  // back
  vec3(-1.0, -1.0, 1.0),
  vec3( 1.0, -1.0, 1.0),

  vec3( 1.0, -1.0, 1.0),
  vec3( 1.0,  1.0, 1.0),

  vec3( 1.0,  1.0, 1.0),
  vec3(-1.0,  1.0, 1.0),

  vec3(-1.0,  1.0, 1.0),
  vec3(-1.0, -1.0, 1.0),

  // front
  vec3(-1.0, -1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),

  vec3( 1.0, -1.0, -1.0),
  vec3( 1.0,  1.0, -1.0),

  vec3( 1.0,  1.0, -1.0),
  vec3(-1.0,  1.0, -1.0),

  vec3(-1.0,  1.0, -1.0),
  vec3(-1.0, -1.0, -1.0),

  // sides
  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0, -1.0,  1.0),

  vec3(1.0, -1.0, -1.0),
  vec3(1.0, -1.0,  1.0),

  vec3(1.0, 1.0, -1.0),
  vec3(1.0, 1.0,  1.0),

  vec3(-1.0, 1.0, -1.0),
  vec3(-1.0, 1.0,  1.0)
);

void main() {
  vec4 node_pos = node_positions[gl_InstanceIndex];

  InnerNode node = nodes[gl_InstanceIndex];

  u32 edge = gl_VertexIndex;
  vec3 pos = positions[edge];
  cellColor = hsl(vec3(node_pos.w / ubo.scene.tree_radius, 0.9, 0.6));

  gl_Position = ubo.scene.worldToScreen * vec4(node_pos.xyz + pos * node_pos.w, 1.0);
}