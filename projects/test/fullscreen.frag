#version 450

#include "shared.h"

layout (std430, binding = 0) uniform UBO {
  Scene scene;
} ubo;


layout (binding = 1) uniform sampler2D logo;

in vec2 o_uv;
out vec4 outColor;

// adapted from: https://www.shadertoy.com/view/XlG3DW
vec2 logo_uv(vec2 pixel_pos) {
  vec2 margin = vec2(30.0);
  vec2 logo_dims = vec2(textureSize(logo, 0));
  vec2 screen_dims = ubo.scene.screen_dims.xy;
  vec2 ratio = (screen_dims - 2.0 * margin) / logo_dims;

  vec2 tex_uv = pixel_pos - margin;
  tex_uv -= .5*logo_dims*max(vec2(ratio.x-ratio.y,ratio.y-ratio.x),0.);
  tex_uv /= logo_dims * min(ratio.x, ratio.y);
  return tex_uv;
}

float offsets[10] = float[10](
  -0.9,
  -0.4,
  -0.2,
  -0.8,
  -0.3,
  -0.7,
  -0.5,
  -0.1,
  -0.95,
  -0.6
);


vec2 hash( vec2 p ) {
  p = vec2(dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)));
  return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

void main() {
  float t = ubo.scene.time;
  float scale = 4;
  vec2 screen_dims = ubo.scene.screen_dims.xy;
  vec2 mouse = ubo.scene.mouse.xy;
  float aspect = screen_dims.x / screen_dims.y;
  vec2 tex_uv = logo_uv(gl_FragCoord.xy) * scale - vec2(scale/2.5);
  vec2 screen_uv = o_uv * vec2(
    aspect,
    1.0
  );

  float partitions = floor(ubo.scene.partitions);
  float time_scale = floor(ubo.scene.time_scale);
  float range = floor(ubo.scene.range);//60.0;

  float inv_partitions = 1.0/partitions;

  float s = 4.0;

  // t = mouse.x * s * range;
  float lt = range - mod(t * time_scale, range * 3.0);
  t = max(lt, 0.0);

  vec2 uv = tex_uv;

  float row = floor(uv.y / inv_partitions);
  float row_offset = offsets[int(row)];

  row_offset = -abs(hash(vec2(row * 10.0, 0.0)).x) * range;

  uv = tex_uv;

  float ot = max(lt - row_offset, 0.0);
  float intensity = (2.0-ot) * ot + row_offset * t;

  //set offsets
  vec2 rOffset = vec2(0.01, 0)*intensity;
  vec2 gOffset = vec2(0.02, 0)*intensity;
  vec2 bOffset = vec2(0.03, 0)*intensity;

  vec4 rValue = texture(logo, uv - rOffset);
  vec4 gValue = texture(logo, uv - gOffset);
  vec4 bValue = texture(logo, uv - bOffset);

  outColor = vec4(rValue.r, gValue.g, bValue.b, 1.0);
}