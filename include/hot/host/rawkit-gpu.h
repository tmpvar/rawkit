#pragma once

#include <rawkit/jit.h>
#include <rawkit/gpu.h>

static void host_init_rawkit_gpu(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_vulkan_find_memory_type", rawkit_vulkan_find_memory_type);
  rawkit_jit_add_export(jit, "rawkit_vulkan_find_queue", rawkit_vulkan_find_queue);
  rawkit_jit_add_export(jit, "rawkit_gpu_queue_ex", rawkit_gpu_queue_ex);
  rawkit_jit_add_export(jit, "rawkit_gpu_queue_submit_ex", rawkit_gpu_queue_submit_ex);

  rawkit_jit_add_export(jit, "rawkit_gpu_default_queue_ex", rawkit_gpu_default_queue_ex);
  rawkit_jit_add_export(jit, "rawkit_gpu_default_command_pool_ex", rawkit_gpu_default_command_pool_ex);

  rawkit_jit_add_export(jit, "rawkit_gpu_command_pool_ex", rawkit_gpu_command_pool_ex);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_create", rawkit_gpu_buffer_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_update", rawkit_gpu_buffer_update);
  rawkit_jit_add_export(jit, "rawkit_gpu_copy_buffer", rawkit_gpu_copy_buffer);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_destroy", rawkit_gpu_buffer_destroy);
  rawkit_jit_add_export(jit, "rawkit_gpu_queue_buffer_for_deletion", rawkit_gpu_queue_buffer_for_deletion);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_transition", rawkit_gpu_buffer_transition);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_map", rawkit_gpu_buffer_map);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_unmap", rawkit_gpu_buffer_unmap);

  rawkit_jit_add_export(jit, "rawkit_gpu_vertex_buffer_create", rawkit_gpu_vertex_buffer_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_vertex_buffer_destroy", rawkit_gpu_vertex_buffer_destroy);
  rawkit_jit_add_export(jit, "rawkit_gpu_create_command_buffer", rawkit_gpu_create_command_buffer);
  rawkit_jit_add_export(jit, "rawkit_gpu_queue_command_buffer_for_deletion", rawkit_gpu_queue_command_buffer_for_deletion);
  rawkit_jit_add_export(jit, "rawkit_gpu_get_tick_idx", rawkit_gpu_queue_command_buffer_for_deletion);

  rawkit_jit_add_export(jit, "rawkit_gpu_fence_create", rawkit_gpu_fence_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_fence_destroy", rawkit_gpu_fence_destroy);
  rawkit_jit_add_export(jit, "rawkit_gpu_fence_status", rawkit_gpu_fence_status);

  // SSBO
  rawkit_jit_add_export(jit, "rawkit_gpu_ssbo_ex", rawkit_gpu_ssbo_ex);
  rawkit_jit_add_export(jit, "rawkit_gpu_ssbo_update", rawkit_gpu_ssbo_update);
  rawkit_jit_add_export(jit, "rawkit_gpu_ssbo_transition", rawkit_gpu_ssbo_transition);

}