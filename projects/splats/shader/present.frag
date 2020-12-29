#version 450

#include "../shared.h"

#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;

layout (std430, binding = 1) buffer FrameBuffer {
  uint64_t framebuffer[];
};

layout (location = 0) out vec4 color;

in vec2 uv;

void main() {

  vec2 dims = ubo.scene.screen_dims.xy;
  vec2 pos = dims * uv;
  uint id = int(pos.x + pos.y * dims.x);

  uint64_t v = framebuffer[id];
  framebuffer[id] = 0xffffffffff000000UL;

  uint ucol = uint(v & 0x00FFFFFFUL);

  if(v >= 0xFFFFFFFFFF000000UL){
    color = vec4(0.0);
    return;
  }

  color = vec4(
    float((ucol & 0xFF0000) >> 16)  / 255.0,
    float((ucol & 0xFF00) >> 8)  / 255.0,
    float((ucol & 0xFF) >> 0)  / 255.0,
    1.0
  );
}