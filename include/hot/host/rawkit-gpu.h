#pragma once

#include <rawkit/jit.h>
#include <rawkit/gpu.h>

void host_init_rawkit_gpu(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_vulkan_find_memory_type", (void *)&rawkit_vulkan_find_memory_type);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_create", (void *)&rawkit_gpu_buffer_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_buffer_destroy", (void *)&rawkit_gpu_buffer_destroy);
  rawkit_jit_add_export(jit, "rawkit_gpu_vertex_buffer_create", (void *)&rawkit_gpu_vertex_buffer_create);
  rawkit_jit_add_export(jit, "rawkit_gpu_vertex_buffer_destroy", (void *)&rawkit_gpu_vertex_buffer_destroy);
}