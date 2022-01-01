#pragma once

#include <rawkit/jit.h>
#include <rawkit/window.h>

static void host_init_rawkit_window(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_window_init", rawkit_window_init);
  rawkit_jit_add_export(jit, "rawkit_window_frame_index", rawkit_window_frame_index);
  rawkit_jit_add_export(jit, "rawkit_window_frame_count", rawkit_window_frame_count);
  rawkit_jit_add_export(jit, "rawkit_window_width", rawkit_window_width);
  rawkit_jit_add_export(jit, "rawkit_window_height", rawkit_window_height);
  rawkit_jit_add_export(jit, "rawkit_glfw_window", rawkit_glfw_window);
}