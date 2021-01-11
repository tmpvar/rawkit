#ifdef CPU_HOST
  #pragma once
  #define GLM_FORCE_RADIANS
  #include <glm/glm.hpp>
  #include <glm/gtc/matrix_transform.hpp>
  #include <glm/gtx/compatibility.hpp>

  #include <glm/glm.hpp>
  using namespace glm;

  typedef uint uint32_t;
#else
  #extension GL_ARB_gpu_shader_int64 : require
  #extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#endif

#define MAX_DEPTH 1000.0


struct Camera {
  vec4 pos;
  // TODO: orientation and vp matrix
};

struct Scene {
  mat4 worldToScreen;
  vec4 screen_dims;
  vec4 eye;
  vec4 brick_dims;
  vec4 cascade_center;
  Camera camera;
  float time;
};

struct DrawIndexedIndirectCommand {
  uint    indexCount;
  uint    instanceCount;
  uint    firstIndex;
  int     vertexOffset;
  uint    firstInstance;
};

// return a bounding rectangle given a square image diameter and a mip.
// see: https://twitter.com/SebAaltonen/status/1327188239451611139
uvec4 image_mip_rect(uint diameter, uint mip) {
  uint pixels_mip = diameter >> mip;
  uvec4 uv_rect = uvec4(0, 0, pixels_mip, pixels_mip);
  if (mip > 0) {
    uv_rect.x = diameter;
    uv_rect.y = diameter - pixels_mip * 2;
  }
  return uv_rect;
}

struct DepthPyramidConstants {
  uvec2 dimensions;
  uint mip;
  uint diameter;	// pow2 y dimension of mip 0 (texture x is 1.5x wider)
};

struct Brick {
  vec4 pos;
};

struct BrickOcclusion {
  uint64_t occlusion[64];
};

#define BRICK_COLOR_DIAMETER 8
#define BRICK_COLOR_LENGTH BRICK_COLOR_DIAMETER * BRICK_COLOR_DIAMETER * BRICK_COLOR_DIAMETER
struct BrickColor {
  // 8^3 which means each voxel is colored by the interpolation
  // between neighboring color values.. It's either this, drop
  // down to 16bit color or figure out a brick level compression scheme. DXT1 should work fine here, but
  // we'll probably want to turn this into a texture!
  uint voxel[512];
};

struct ray_hit_t {
  vec4 color;
  vec3 pos;
  vec3 normal;
  float density;
  float iterations;
};

struct scene_t {
  vec3 grid_dims;
  vec3 inv_grid_dims;
  Brick brick;
};

struct bound3_t {
    vec3 mMin;
    vec3 mMax;
};


#define OP_ID_BEZIER 1.0
#define OP_ID_SPHERE 2.0

struct op_t {
  vec4 control; // id, nop, nop, nop
  vec4 a;       // x, y, z, radius
  vec4 b;       // x, y, z, radius
  vec4 c;       // x, y, z, radius
  vec4 d;       // x, y, z, radius
};




float dot2( vec3 v ) { return dot(v,v); }

vec2 sdBezier(vec3 pos, vec3 A, vec3 B, vec3 C)
{
    vec3 a = B - A;
    vec3 b = A - 2.0f*B + C;
    vec3 c = a * 2.0f;
    vec3 d = A - pos;

    float kk = 1.0f / dot(b,b);
    float kx = kk * dot(a,b);
    float ky = kk * (2.0f*dot(a,a)+dot(d,b)) / 3.0f;
    float kz = kk * dot(d,a);

    vec2 res;

    float p = ky - kx*kx;
    float p3 = p*p*p;
    float q = kx*(2.0f*kx*kx - 3.0f*ky) + kz;
    float h = q*q + 4.0f*p3;

    if(h >= 0.0)
    {
        h = sqrt(h);
        vec2 x = (vec2(h, -h) - q) / 2.0f;
        vec2 uv = sign(x)*pow(abs(x), vec2(1.0f/3.0f));
        float t = clamp(uv.x+uv.y-kx, 0.0f, 1.0f);

        // 1 root
        res = vec2(dot2(d+(c+b*t)*t),t);
    }
    else
    {
        float z = sqrt(-p);
        float v = acos( q/(p*z*2.0f) ) / 3.0f;
        float m = cos(v);
        float n = sin(v)*1.732050808f;
        vec3 t = clamp( vec3(m+m,-n-m,n-m)*z-kx, 0.0f, 1.0f);

        // 3 roots, but only need two
        float dis = dot2(d+(c+b*t.x)*t.x);
        res = vec2(dis,t.x);

        dis = dot2(d+(c+b*t.y)*t.y);
        if( dis<res.x ) res = vec2(dis,t.y );
    }

    res.x = sqrt(res.x);
    return res;
}