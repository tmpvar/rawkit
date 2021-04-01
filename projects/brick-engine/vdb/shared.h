// shared data structures between GPU and CPU
#ifndef _SHARED_H
  #define _SHARED_H

  #ifdef CPU_HOST
      #include <rawkit/rawkit.h>
      #include <glm/glm.hpp>
      using namespace glm;
  #else
      #extension GL_EXT_shader_explicit_arithmetic_types : enable
      #define u32 uint32_t
      #define i32 int32_t
      #define u8 uint8_t
  #endif

  struct Scene {
    vec4 screen_dims;
    mat4 worldToScreen;
    mat4 view;
    mat4 proj;
    vec4 eye;
    vec4 mouse;

    vec3 tree_center;
    float tree_radius;

    // effectively tan(radians(fov) / 2.0)
    float pixel_size;

    u32 data_size;
  };
#endif