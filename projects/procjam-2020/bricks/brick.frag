#version 450
#include "shared.h"
layout (location = 0) out vec4 color;
layout (std430, binding = 1) uniform UBO {
  Scene scene;
} ubo;

in vec4 v_color;

void main() {
  color = v_color;//vec4(1.0, 0.0, 1.0, 1.0);
}
