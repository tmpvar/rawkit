#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in mat4 inMat4;
layout(location = 5) in vec4 inColor;
layout (std430) uniform ubo { float ubo_float; };
void main() {
  gl_Position = vec4(inPosition, 0.0, 1.0);
}