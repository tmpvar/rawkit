#version 450
#include "shared.h"

layout (location = 0) out vec4 color;
layout (std430, binding = 1) uniform UBO {
  Scene scene;
} ubo;

layout(std430, binding = 2) readonly buffer Bricks {
  Brick bricks[];
};

layout(std430, binding = 3) readonly buffer Ops {
  op_t ops[];
};

in vec3 rayOrigin;
in vec3 normal;
flat in vec3 eye;
flat in uint brick_id;

vec4 bezier_at_t(vec4 p1, vec4 p2, vec4 p3, vec4 p4, float t) {
  vec4 a = mix(p1, p2, t);
  vec4 b = mix(p2, p3, t);
  vec4 c = mix(p3, p4, t);

  return mix(
    mix(a, b, t),
    mix(b, c, t),
    t
  );
}

float length2( in vec3 v ) { return dot(v,v); }


vec3 iSegment( in vec3 ro, in vec3 rd, in vec3 a, in vec3 b )
{
	vec3 ba = b - a;
	vec3 oa = ro - a;

	float oad  = dot( oa, rd );
	float dba  = dot( rd, ba );
	float baba = dot( ba, ba );
	float oaba = dot( oa, ba );

	vec2 th = vec2( -oad*baba + dba*oaba, oaba - oad*dba ) / (baba - dba*dba);

	th.x = max(   th.x, 0.0 );
	th.y = clamp( th.y, 0.0, 1.0 );

	vec3 p =  a + ba*th.y;
	vec3 q = ro + rd*th.x;

	return vec3( th, length2( p-q ) );

}


float iBezier( in vec3 ro, in vec3 rd, in vec3 p0, in vec3 p1, in vec3 p2, in vec3 p3, in float width)
{
    const int kNum = 50;

    float hit = -1.0;
    float res = 1e10;
    vec3 a = p0;
    for( int i=1; i<kNum; i++ )
    {
        float t = float(i)/float(kNum-1);
        float s = 1.0-t;
        vec3 b = p0*s*s*s + p1*3.0*s*s*t + p2*3.0*s*t*t + p3*t*t*t;
        vec3 r = iSegment( ro, rd, a, b );
        if( r.z<width*width )
        {
            res = min( res, r.x );
            hit = 1.0;
        }
        a = b;
    }

    return res*hit;


}


float smin( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    return min( a, b ) - h*h*k*(1.0/4.0);
}


float brick_march(in Brick brick, in vec3 rayOrigin, in vec3 rayDir, out vec3 normal, out float iterations, out vec3 pos) {
  rayOrigin -= rayDir;
	pos = floor(rayOrigin);
	vec3 dir = sign(rayDir);
	vec3 sideDist = pos + 0.5 + dir * 0.5 - rayOrigin;
  vec3 invDir = 1.0 / rayDir;
	vec3 dt = sideDist * invDir;

  vec3 brick_dims = vec3(brick.pos.w);


  float max_iterations = length(brick_dims);
  for (iterations = 0; iterations < max_iterations; iterations++) {
    vec3 mm = step(vec3(dt.xyz), vec3(dt.yxy)) * step(vec3(dt.xyz), vec3(dt.zzx));
    sideDist = mm * dir;

    pos += sideDist;
    dt += sideDist * invDir;



    if (all(greaterThanEqual(pos, vec3(0.0))) && all(lessThan(pos, brick_dims))) {


      float d = 10000000.0;
      for (uint i=0; i<brick.params.x; i++) {
        op_t op = ops[i];

        if (op.control.x == OP_ID_BEZIER) {
          vec3 p = brick.pos.xyz * brick_dims + pos;
          vec2 r = sdBezier(
            p - op.a.xyz,
            op.b.xyz,
            op.c.xyz,
            op.d.xyz
          );

          vec4 v = bezier_at_t(op.a, op.b, op.c, op.d, r.y);
          d = smin(d, r.x - v.w, 40.1);
        } else if (op.control.x == OP_ID_SPHERE) {
          d = min(
            d,
            distance(brick.pos.xyz * brick_dims + pos, op.a.xyz) -  op.a.w
          );

        }
      }
      if (d <= 0.0) {
        normal = -sideDist;
        return 1.0;
      }

      // vec3 center = brick_dims * 0.5;
      // if (distance(floor(pos) + 0.5, center) - length(brick_dims) * 0.35 < 0.0) {
      //   normal = -sideDist;
      //   return 1.0;
      // }
    }
  }

  return 0.0;
}

void main() {
  Brick brick = bricks[brick_id];
  vec3 brick_dims = vec3(brick.pos.w);

  vec3 origin = rayOrigin * brick_dims;
  vec3 pos;
  vec3 dir = normalize(rayOrigin - eye);

  vec3 normal;
  float iterations;

  float hit = brick_march(
    brick,
    origin,
    dir,
    normal,
    iterations,
    pos
  );

  float depth = mix(
    1.0,
    (distance(rayOrigin + brick.pos.xyz, ubo.scene.eye.xyz) / 10000.0),
    hit
  );

  gl_FragDepth = depth;

  if (hit == 0.0) {
    color = vec4(rayOrigin, 1.0);
    discard;
  } else {
    color = vec4((normal + 1.0) * 0.5, 1.0);
  }


}
