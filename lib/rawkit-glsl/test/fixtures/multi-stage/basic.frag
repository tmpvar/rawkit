#version 450
layout(location = 0) out vec3 fragColor;
layout (std430) uniform ubo { float ubo_float; };
layout (std430, binding = 1) uniform UBO2 { float ubo_float; } ubo2;
void main() {
  fragColor = vec3(1.0, 0.0, 1.0);
}