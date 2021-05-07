#version 450

layout (location = 0) out vec4 outColor;
layout (binding = 1) uniform sampler2D tex;

in vec2 uv;

vec3 hsl( in vec3 c ) {
  vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
  return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

void main() {
  float d = texture(tex, uv).r;
  float offset = 0.0;
  vec3 c = hsl(vec3(d * 70.0, 0.9, 0.6));
  outColor = vec4(c, 1.0);

}