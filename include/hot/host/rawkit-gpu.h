#pragma once

#include <rawkit/jit.h>
#include <rawkit/gpu.h>

static void host_init_rawkit_gpu(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_vulkan_find_memory_type", (void *)&rawkit_vulkan_find_memory_type);
  rawkit_jit_add_export(jit, "rawkit_vulkan_find_queue", (void *)&rawkit_vulkan_find_queue);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_create", (void *)&rawkit_gpu_buffer_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_update", (void *)&rawkit_gpu_buffer_update);
  rawkit_jit_add_export(jit, "rawkit_gpu_copy_buffer", (void *)&rawkit_gpu_copy_buffer);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_destroy", (void *)&rawkit_gpu_buffer_destroy);
  rawkit_jit_add_export(jit, "rawkit_gpu_queue_buffer_for_deletion", (void *)&rawkit_gpu_queue_buffer_for_deletion);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_transition", (void *)&rawkit_gpu_buffer_transition);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_map", (void *)&rawkit_gpu_buffer_map);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_unmap", (void *)&rawkit_gpu_buffer_unmap);

  rawkit_jit_add_export(jit, "rawkit_gpu_vertex_buffer_create", (void *)&rawkit_gpu_vertex_buffer_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_vertex_buffer_destroy", (void *)&rawkit_gpu_vertex_buffer_destroy);
  rawkit_jit_add_export(jit, "rawkit_gpu_create_command_buffer", (void *)&rawkit_gpu_create_command_buffer);
  rawkit_jit_add_export(jit, "rawkit_gpu_queue_command_buffer_for_deletion", (void *)&rawkit_gpu_queue_command_buffer_for_deletion);
  rawkit_jit_add_export(jit, "rawkit_gpu_get_tick_idx", (void *)&rawkit_gpu_queue_command_buffer_for_deletion);

  rawkit_jit_add_export(jit, "rawkit_gpu_fence_create", (void *)&rawkit_gpu_fence_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_fence_destroy", (void *)&rawkit_gpu_fence_destroy);
  rawkit_jit_add_export(jit, "rawkit_gpu_fence_status", (void *)&rawkit_gpu_fence_status);

  // SSBO
  rawkit_jit_add_export(jit, "rawkit_gpu_ssbo_ex", (void *)&rawkit_gpu_ssbo_ex);
  rawkit_jit_add_export(jit, "rawkit_gpu_ssbo_update", (void *)&rawkit_gpu_ssbo_update);
  rawkit_jit_add_export(jit, "rawkit_gpu_ssbo_transition", (void *)&rawkit_gpu_ssbo_transition);

}