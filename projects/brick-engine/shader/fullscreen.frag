#version 450

#include "../shared.h"

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
  DepthPyramidConstants depth_pyramid;
} push;

layout (binding = 1) uniform sampler2D tex_color;
layout (binding = 2, r32ui) uniform uimage2D culling_debug_tex;

in vec2 uv;

void main() {


  // vec3 color = vec3(0.0);
  // for (uint mip = 0; mip < 7; ++mip) {
  //   uvec4 mip_rect = image_mip_rect(push.depth_pyramid.diameter, mip);
  //   ivec2 mip_uv = ivec2(mip_rect.xy) + ivec2(uv * vec2(mip_rect.zw));
  //   uint i = imageLoad(culling_debug_tex, mip_uv).x;

  //   float v = float(i) * 0.25;
  //   uint channel = mip % 3;
  //   if (channel == 0) outColor.r += v;
  //   else if (channel == 1) outColor.g += v;
  //   else if (channel == 2) outColor.b += v;
  // }

  if (uv.x >= 0.5) {
    outColor = vec4(
      float(imageLoad(culling_debug_tex, ivec2((uv - vec2(0.5, 0.0)) * vec2(2.0, 1.0) * gl_FragCoord.xy)).x),
      0.0,
      0.0,
      1.0
    );
  } else {
    outColor = texture(tex_color, uv * vec2(2.0, 1.0));
  }

}