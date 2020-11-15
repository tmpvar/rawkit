#pragma once

#include <vulkan/vulkan.h>
#include <rawkit/core.h>
#include <rawkit/mesh.h>
#include <string.h>

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct rawkit_gpu_t {
  RAWKIT_RESOURCE_FIELDS

  VkPhysicalDevice physical_device;
  VkDevice device;
  VkAllocationCallbacks *allocator;
  VkPipelineCache pipeline_cache;
  VkCommandPool command_pool;

  // Queues
  uint32_t queue_count;
  VkQueueFamilyProperties *queue_family_properties;
} rawkit_gpu_t;

typedef struct rawkit_gpu_buffer_t {
  VkDeviceMemory memory;
  VkBuffer handle;
} rawkit_gpu_buffer_t;

typedef struct rawkit_gpu_vertex_buffer_t {
  RAWKIT_RESOURCE_FIELDS

  rawkit_gpu_buffer_t *vertices;
  rawkit_gpu_buffer_t *indices;
} rawkit_gpu_vertex_buffer_t;

rawkit_gpu_t *rawkit_default_gpu();
void rawkit_set_default_gpu(rawkit_gpu_t *gpu);

int32_t rawkit_vulkan_find_queue_family_index(rawkit_gpu_t *gpu, VkQueueFlags flags);
VkQueue rawkit_vulkan_find_queue(rawkit_gpu_t *gpu, VkQueueFlags flags);

uint32_t rawkit_vulkan_find_memory_type(rawkit_gpu_t *gpu, VkMemoryPropertyFlags properties, uint32_t type_bits);


rawkit_gpu_buffer_t *rawkit_gpu_buffer_create(
  rawkit_gpu_t *gpu,
  VkDeviceSize size,
  VkMemoryPropertyFlags memory_flags,
  VkBufferUsageFlags buffer_usage_flags
);

VkResult rawkit_gpu_buffer_update(
  rawkit_gpu_t *gpu,
  rawkit_gpu_buffer_t *dst,
  void *src,
  VkDeviceSize size
);

VkResult rawkit_gpu_buffer_destroy(rawkit_gpu_t *gpu, rawkit_gpu_buffer_t *buf);

rawkit_gpu_vertex_buffer_t *rawkit_gpu_vertex_buffer_create(
  rawkit_gpu_t *gpu,
  VkQueue queue,
  VkCommandPool pool,
  rawkit_mesh_t *mesh
);

VkResult rawkit_gpu_vertex_buffer_destroy(rawkit_gpu_t *gpu, rawkit_gpu_vertex_buffer_t *buf);

#ifdef __cplusplus
  }
#endif
