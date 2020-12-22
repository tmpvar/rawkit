#ifdef CPU_HOST
  #define GLM_FORCE_RADIANS
  #include "../glm/glm.hpp"
  #include "../glm/gtc/matrix_transform.hpp"
  #include "../glm/gtx/compatibility.hpp"

  #include "../glm/glm.hpp"
  using namespace glm;

  typedef uint uint32_t;
#else
  #extension GL_ARB_gpu_shader_int64 : require
  #extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#endif

struct Camera {
  vec4 pos;
  // TODO: orientation and vp matrix
};

struct Scene {
  mat4 worldToScreen;
  vec4 screen_dims;
  vec4 eye;
  vec4 brick_dims;
  Camera camera;
  float time;
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
  // down to 16bit color or figure out a brick level compression scheme.
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
