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
  #extension GL_ARB_gpu_shader_int64 : enable
  #extension GL_NV_shader_atomic_int64 : enable
#endif


struct Point {
  vec4 normal;
  float x;
  float y;
  float z;
  uint color;
};

// poorly named but is a housing for an atomic counter
struct PointState {
  uvec3 dispatch;
};

struct Scene {
  mat4 worldToScreen;
  vec4 screen_dims;
  vec4 eye;
  vec4 brick_dims;
  float time;
};
