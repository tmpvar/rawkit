#ifdef CPU_HOST
  #define GLM_FORCE_RADIANS
  #include "glm/glm.hpp"
  #include "glm/gtc/matrix_transform.hpp"
  #include "glm/gtx/compatibility.hpp"

  #include "glm/glm.hpp"
  using namespace glm;

  typedef uint uint32_t;
#endif

struct world_ubo_t {
  mat4 worldToScreen;
  vec4 eye;
  vec4 world_dims;
  float time;
};

struct light_t {
  vec3 pos;
  vec3 color;
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
};

#define WATER_ALPHA 0.1f
#define ROCK_ALPHA 0.2f
#define DIRT_ALPHA 0.3f
#define PLANT_ALPHA 0.4f

struct visible_voxel {
  uvec4 pos;
  vec4 face_color[6];
};
