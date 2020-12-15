#pragma once

#include <rawkit/jit.h>
#include <rawkit/gpu.h>

void host_init_rawkit_gpu(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_vulkan_find_memory_type", (void *)&rawkit_vulkan_find_memory_type);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_create", (void *)&rawkit_gpu_buffer_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_update", (void *)&rawkit_gpu_buffer_update);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_destroy", (void *)&rawkit_gpu_buffer_destroy);
  rawkit_jit_add_export(jit, "rawkit_gpu_vertex_buffer_create", (void *)&rawkit_gpu_vertex_buffer_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_vertex_buffer_destroy", (void *)&rawkit_gpu_vertex_buffer_destroy);
  rawkit_jit_add_export(jit, "rawkit_default_gpu", (void *)&rawkit_default_gpu);
  rawkit_jit_add_export(jit, "rawkit_gpu_create_command_buffer", (void *)&rawkit_gpu_create_command_buffer);
  rawkit_jit_add_export(jit, "rawkit_gpu_queue_command_buffer_for_deletion", (void *)&rawkit_gpu_queue_command_buffer_for_deletion);
  rawkit_jit_add_export(jit, "rawkit_gpu_get_tick_idx", (void *)&rawkit_gpu_queue_command_buffer_for_deletion);

  // SSBO
  rawkit_jit_add_export(jit, "_rawkit_gpu_ssbo", (void *)&_rawkit_gpu_ssbo);
  rawkit_jit_add_export(jit, "rawkit_gpu_ssbo_update", (void *)&rawkit_gpu_ssbo_update);

}