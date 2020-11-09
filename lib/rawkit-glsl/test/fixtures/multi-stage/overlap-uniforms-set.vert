#version 450
layout(location = 0) in vec2 inPosition;
layout (std430) uniform UBO { vec4 color;  mat4 mvp; } uniforms;
layout (std430, binding = 1) uniform Offsets { vec4 vec[10]; } offsets;
void main() {
  gl_Position = vec4(inPosition, 0.0, 1.0);
}