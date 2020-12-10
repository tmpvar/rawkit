#ifdef CPU_HOST
  #include "glm/glm.hpp"
  using namespace glm;
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

#define WATER_ALPHA 0.0f
#define ROCK_ALPHA 0.1f
#define DIRT_ALPHA 0.2f
#define PLANT_ALPHA 0.3f
