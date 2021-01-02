#ifdef CPU_HOST
   #pragma once
  #define GLM_FORCE_RADIANS
  #include <glm/glm.hpp>
  #include <glm/gtc/matrix_transform.hpp>
  #include <glm/gtx/compatibility.hpp>

  #include <glm/glm.hpp>
  using namespace glm;

  typedef uint uint32_t;
#endif



struct Scene {
  // mat4 worldToScreen;
  vec4 screen_dims;
  vec4 mouse;
  float partitions;
  float time_scale;
  float range;

  // vec4 eye;
  // vec4 brick_dims;
  float time;

};
