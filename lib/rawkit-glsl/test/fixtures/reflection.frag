#version 450
#extension GL_EXT_ray_tracing : require
in vec4 in_color;
in float in_float;
out vec4 out_color;
out float out_float;
layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputDepth;
layout(binding = 1, set = 0) uniform accelerationStructureEXT acc;
void main() {
  out_color = in_color;
  out_float = in_float;
}