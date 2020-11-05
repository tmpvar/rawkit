#pragma once

#include <vulkan/vulkan.h>
#include <rawkit/glsl.h>

#include <string.h>

#ifdef __cplusplus
  extern "C" {
#endif


typedef struct rawkit_gpu_pipeline_t {

} rawkit_gpu_pipeline_t;

rawkit_gpu_pipeline_t *rawkit_gpu_pipeline(
  const char *name
);



typedef struct rawkit_gpu_buffer_t {
  VkDeviceMemory memory;
  VkBuffer handle;
} rawkit_gpu_buffer_t;

typedef struct rawkit_gpu_vertex_buffer_t {
  rawkit_gpu_buffer_t *vertices;
} rawkit_gpu_vertex_buffer_t;

uint32_t rawkit_vulkan_find_memory_type(VkPhysicalDevice physical_device, VkMemoryPropertyFlags properties, uint32_t type_bits);

rawkit_gpu_buffer_t *rawkit_gpu_buffer_create(
  VkPhysicalDevice physical_device,
  VkDevice device,
  VkDeviceSize size,
  VkMemoryPropertyFlags flags
);

VkResult rawkit_gpu_buffer_destroy(VkDevice device, rawkit_gpu_buffer_t *buf);


rawkit_gpu_vertex_buffer_t *rawkit_gpu_vertex_buffer_create(
  VkPhysicalDevice physical_device,
  VkDevice device,
  VkQueue queue,
  VkCommandPool pool,
  uint32_t vertex_count,
  float *vertices
);
VkResult rawkit_gpu_vertex_buffer_destroy(VkDevice device, rawkit_gpu_vertex_buffer_t *buf);

#ifdef __cplusplus
  }
#endif
