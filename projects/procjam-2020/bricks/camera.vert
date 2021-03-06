#version 450

#include "shared.h"

layout (std430, binding = 1) uniform UBO {
  Scene scene;
} ubo;


// // a cube
// const vec3 positions[36] = vec3[36](
//   vec3(0.0, 0.0, 1.0),
//   vec3(1.0, 1.0, 1.0),
//   vec3(1.0, 0.0, 1.0),

//   vec3(0.0, 0.0, 1.0),
//   vec3(0.0, 1.0, 1.0),
//   vec3(1.0, 1.0, 1.0),

//   vec3(1.0, 1.0, 1.0),
//   vec3(1.0, 1.0, 0.0),
//   vec3(1.0, 0.0, 0.0),

//   vec3(1.0, 1.0, 1.0),
//   vec3(1.0, 0.0, 0.0),
//   vec3(1.0, 0.0, 1.0),

//   vec3(0.0, 0.0, 0.0),
//   vec3(1.0, 0.0, 0.0),
//   vec3(1.0, 1.0, 0.0),

//   vec3(0.0, 0.0, 0.0),
//   vec3(1.0, 1.0, 0.0),
//   vec3(0.0, 1.0, 0.0),

//   vec3(0.0, 0.0, 0.0),
//   vec3(0.0, 1.0, 1.0),
//   vec3(0.0, 0.0, 1.0),

//   vec3(0.0, 0.0, 0.0),
//   vec3(0.0, 1.0, 0.0),
//   vec3(0.0, 1.0, 1.0),

//   vec3(1.0, 1.0, 1.0),
//   vec3(0.0, 1.0, 1.0),
//   vec3(0.0, 1.0, 0.0),

//   vec3(1.0, 1.0, 1.0),
//   vec3(0.0, 1.0, 0.0),
//   vec3(1.0, 1.0, 0.0),

//   vec3(0.0, 0.0, 0.0),
//   vec3(1.0, 0.0, 1.0),
//   vec3(1.0, 0.0, 0.0),

//   vec3(0.0, 0.0, 0.0),
//   vec3(0.0, 0.0, 1.0),
//   vec3(1.0, 0.0, 1.0)
// );


// void main() {
//   vec3 pos = positions[gl_VertexIndex] + ubo.scene.camera.pos.xyz;
// 	gl_Position = ubo.scene.worldToScreen * vec4(pos, 1.0);
// }


layout (location = 0) in vec3 aPosition;

void main() {
  gl_Position = ubo.scene.worldToScreen * vec4(aPosition * 0.025 + ubo.scene.camera.pos.xyz , 1.0);
}