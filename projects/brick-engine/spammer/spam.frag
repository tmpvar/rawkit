#version 450
out vec4 outColor;

in vec3 rayOrigin;
in vec3 normal;
in vec3 worldPos;
flat in vec3 eye;
flat in uint brick_id;

vec3 hsl( in vec3 c ) {
  vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
  return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

void main() {

  vec3 m = vec3(0.0);
  if (normal.y > 0.0) {
    m = vec3(1.0);
  } else if (normal.x != 0.0) {
    m = vec3(0.7);
  } else if (normal.z != 0.0) {
    m = vec3(-0.9);
  } else {
    m = vec3(0.2);
  }

  float l = length(vec2(127));
  float d = distance(worldPos.xz, vec2(127, 127));

  outColor = vec4(
    hsl(vec3(0.4 + d / l , 0.9, 0.3)) + m * 0.015,
    1.0
  );
}