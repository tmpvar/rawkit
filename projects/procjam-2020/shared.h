#ifdef CPU_HOST
  #include "glm/glm.hpp"
  using namespace glm;
#endif

struct world_ubo_t {
  mat4 worldToScreen;
  vec4 eye;
  vec4 world_dims;
};