#version 450

#include "shared.h"

layout (std430, binding = 1) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 2) readonly buffer Bodies {
  BodyProxy bodies[];
};

out vec3 normal;

// a cube
const vec3 positions[36] = vec3[36](
  vec3(-0.5, -0.5, 0.5),
  vec3(0.5, 0.5, 0.5),
  vec3(0.5, -0.5, 0.5),

  vec3(-0.5, -0.5, 0.5),
  vec3(-0.5, 0.5, 0.5),
  vec3(0.5, 0.5, 0.5),

  vec3(0.5, 0.5, 0.5),
  vec3(0.5, 0.5, -0.5),
  vec3(0.5, -0.5, -0.5),

  vec3(0.5, 0.5, 0.5),
  vec3(0.5, -0.5, -0.5),
  vec3(0.5, -0.5, 0.5),

  vec3(-0.5, -0.5, -0.5),
  vec3(0.5, -0.5, -0.5),
  vec3(0.5, 0.5, -0.5),

  vec3(-0.5, -0.5, -0.5),
  vec3(0.5, 0.5, -0.5),
  vec3(-0.5, 0.5, -0.5),

  vec3(-0.5, -0.5, -0.5),
  vec3(-0.5, 0.5, 0.5),
  vec3(-0.5, -0.5, 0.5),

  vec3(-0.5, -0.5, -0.5),
  vec3(-0.5, 0.5, -0.5),
  vec3(-0.5, 0.5, 0.5),

  vec3(0.5, 0.5, 0.5),
  vec3(-0.5, 0.5, 0.5),
  vec3(-0.5, 0.5, -0.5),

  vec3(0.5, 0.5, 0.5),
  vec3(-0.5, 0.5, -0.5),
  vec3(0.5, 0.5, -0.5),

  vec3(-0.5, -0.5, -0.5),
  vec3(0.5, -0.5, 0.5),
  vec3(0.5, -0.5, -0.5),

  vec3(-0.5, -0.5, -0.5),
  vec3(-0.5, -0.5, 0.5),
  vec3(0.5, -0.5, 0.5)
);

vec3 normals[6] = vec3[6](
  vec3(-1.0,  0.0,  0.0),
  vec3( 0.0, -1.0,  0.0),
  vec3( 0.0,  0.0, -1.0),
  vec3( 1.0,  0.0,  0.0),
  vec3( 0.0,  1.0,  0.0),
  vec3( 0.0,  0.0,  1.0)
);

// Quaternion multiplication
// http://mathworld.wolfram.com/Quaternion.html
vec4 qmul(vec4 q1, vec4 q2) {
	return vec4(
		q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz),
		q1.w * q2.w - dot(q1.xyz, q2.xyz)
	);
}

// Vector rotation with a quaternion
// http://mathworld.wolfram.com/Quaternion.html
vec3 rotate_vector(vec3 v, vec4 r) {
	vec4 r_c = r * vec4(-1, -1, -1, 1);
	return qmul(r, qmul(vec4(v, 0), r_c)).xyz;
}

void main() {
  BodyProxy body = bodies[gl_InstanceIndex];
  vec3 pos = rotate_vector(positions[gl_VertexIndex], body.rot);
  normal = rotate_vector(normals[gl_VertexIndex / 3 / 2], body.rot);
	gl_Position = ubo.scene.worldToScreen * vec4(
    pos * body.size.xyz + body.pos.xyz,
    1.0
  );

}