#version 450
#include "../shared.h"
#include "uberprim.glsl"

layout (location = 0) out vec4 color;
layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

in vec3 rayOrigin;
flat in vec3 eye;
flat in uint brick_id;
flat in float mip;

vec3 brick_dims = vec3(64.0);

void main() {
  vec3 pos = rayOrigin * brick_dims;
  vec3 dir = normalize(rayOrigin - eye);
  color = vec4(rayOrigin, 1.0);
}
