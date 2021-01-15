#pragma once

#include <rawkit/core.h>
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rawkit_window_t {
  RAWKIT_RESOURCE_FIELDS

  // internal
  void *_state;
} rawkit_window_t;


void rawkit_window_init(int width, int height, const char *title);

uint32_t rawkit_window_frame_index();
uint32_t rawkit_window_frame_count();
uint32_t rawkit_window_width();
uint32_t rawkit_window_height();

GLFWwindow *rawkit_glfw_window();

#ifdef __cplusplus
}
#endif
