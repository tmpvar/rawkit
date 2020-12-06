#pragma once

#include <vulkan/vulkan.h>
#include <rawkit/core.h>
#include <rawkit/mesh.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
  extern "C" {
#endif

// TOOD: migrate these in main.cpp
// static uint32_t                 g_QueueFamily = (uint32_t)-1;
// static VkQueue                  g_Queue = VK_NULL_HANDLE;

typedef struct rawkit_gpu_t {
  RAWKIT_RESOURCE_FIELDS

  bool valid;

  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkAllocationCallbacks *allocator;
  VkPipelineCache pipeline_cache;
  VkCommandPool command_pool;

  VkDebugReportCallbackEXT debug_report;

  // Queues
  uint32_t queue_count;
  VkQueueFamilyProperties *queue_family_properties;

  VkDescriptorPool default_descriptor_pool;
  int32_t graphics_queue_family_index;
  VkQueue graphics_queue;

  // internal
  void *_state;
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

rawkit_gpu_t *rawkit_gpu_init(
  const char** extensions,
  uint32_t extensions_count,
  bool validation,
  PFN_vkDebugReportCallbackEXT debug_callback
);

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



VkCommandBuffer rawkit_gpu_create_command_buffer(rawkit_gpu_t *gpu);

void rawkit_gpu_tick(rawkit_gpu_t *gpu);
void rawkit_gpu_queue_command_buffer_for_deletion(rawkit_gpu_t *gpu, VkCommandBuffer buffer, VkFence fence, VkCommandPool pool);
uint32_t rawkit_gpu_get_tick_idx(rawkit_gpu_t *gpu);

#ifdef __cplusplus
  }
#endif
