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
  VkPhysicalDeviceProperties2 physical_device_properties;
  VkPhysicalDeviceSubgroupProperties physical_device_subgroup_properties;
  VkDevice device;
  VkAllocationCallbacks *allocator;
  VkPipelineCache pipeline_cache;
  VkCommandPool command_pool;

  VkDebugReportCallbackEXT debug_report;

  // Queues
  uint32_t queue_count;
  VkQueueFamilyProperties *queue_family_properties;

  VkDescriptorPool default_descriptor_pool;
  VkQueue graphics_queue;

  // internal
  void *_state;
} rawkit_gpu_t;

typedef struct rawkit_gpu_buffer_t {
  RAWKIT_RESOURCE_FIELDS

  VkDeviceMemory memory;
  VkBuffer handle;
  VkDeviceSize size;

  VkAccessFlags access;

  rawkit_gpu_t *gpu;
  VkMemoryPropertyFlags memory_flags;
  VkBufferUsageFlags buffer_usage_flags;
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

int32_t rawkit_vulkan_find_queue_family_index(rawkit_gpu_t *gpu, VkQueueFlags flags);
VkQueue rawkit_vulkan_find_queue(rawkit_gpu_t *gpu, VkQueueFlags flags);

typedef struct rawkit_gpu_queue_t {
  VkQueue handle;
  i32 family_idx;
  VkCommandPool command_pool;

  // internal
  void *_state;
} rawkit_gpu_queue_t;

// Note: inside of a worker, the queue is shared with the root thread, but all threads synchronise
rawkit_gpu_queue_t rawkit_gpu_queue_ex(rawkit_gpu_t *gpu, VkQueueFlags flags, u32 queue_idx);


#define rawkit_gpu_queue(flags) rawkit_gpu_queue_ex(rawkit_default_gpu(), flags, 0)

// choose the queue with the largest number of flags
rawkit_gpu_queue_t rawkit_gpu_default_queue_ex(rawkit_gpu_t *gpu);
#define rawkit_gpu_default_queue() rawkit_gpu_default_queue_ex(rawkit_default_gpu())

// retrieve the command_pool associated with the queue that has
// the largest number of flags.
VkCommandPool rawkit_gpu_default_command_pool_ex(rawkit_gpu_t *gpu);
#define rawkit_gpu_default_command_pool() rawkit_gpu_default_command_pool_ex(rawkit_default_gpu())

// threadsafe enqueing of command buffers into the specified rawkit queue
VkResult rawkit_gpu_queue_submit_ex(
  rawkit_gpu_queue_t *queue,
  VkCommandBuffer command_buffer,
  VkFence fence
);

#define rawkit_gpu_queue_submit rawkit_gpu_queue_submit_ex

uint32_t rawkit_vulkan_find_memory_type(rawkit_gpu_t *gpu, VkMemoryPropertyFlags properties, uint32_t type_bits);

rawkit_gpu_buffer_t *rawkit_gpu_buffer_create(
  const char *name,
  rawkit_gpu_t *gpu,
  VkDeviceSize size,
  VkMemoryPropertyFlags memory_flags,
  VkBufferUsageFlags buffer_usage_flags
);

VkResult rawkit_gpu_buffer_update(
  rawkit_gpu_buffer_t *dst,
  void *src,
  VkDeviceSize size
);

VkResult rawkit_gpu_buffer_transition(
  rawkit_gpu_buffer_t* buffer,
  VkCommandBuffer command_buffer,
  VkPipelineStageFlags src_stage,
  VkPipelineStageFlags dest_stage,
  VkBufferMemoryBarrier extend
);

VkFence rawkit_gpu_copy_buffer(
  rawkit_gpu_t *gpu,
  VkQueue queue,
  VkCommandPool pool,
  rawkit_gpu_buffer_t *src,
  rawkit_gpu_buffer_t *dst,
  VkDeviceSize src_offset,
  VkDeviceSize dst_offset,
  VkDeviceSize size
);

void rawkit_gpu_queue_buffer_for_deletion(rawkit_gpu_buffer_t *buffer);
VkResult rawkit_gpu_buffer_destroy(rawkit_gpu_t *gpu, rawkit_gpu_buffer_t *buf);

rawkit_gpu_vertex_buffer_t *rawkit_gpu_vertex_buffer_create(
  rawkit_gpu_t *gpu,
  VkQueue queue,
  VkCommandPool pool,
  rawkit_mesh_t *mesh
);

VkResult rawkit_gpu_vertex_buffer_destroy(rawkit_gpu_t *gpu, rawkit_gpu_vertex_buffer_t *buf);

VkCommandPool rawkit_gpu_command_pool_ex(rawkit_gpu_t *gpu, VkQueueFlags flags);
#define rawkit_gpu_command_pool(flags) rawkit_gpu_command_pool_ex(rawkit_default_gpu(), flags)

VkCommandBuffer rawkit_gpu_create_command_buffer(rawkit_gpu_t *gpu, VkCommandPool pool);

void rawkit_gpu_tick(rawkit_gpu_t *gpu);
void rawkit_gpu_queue_command_buffer_for_deletion(rawkit_gpu_t *gpu, VkCommandBuffer buffer, VkFence fence, VkCommandPool pool);
uint32_t rawkit_gpu_get_tick_idx(rawkit_gpu_t *gpu);


VkResult rawkit_gpu_fence_create(rawkit_gpu_t *gpu, VkFence *fence);
void rawkit_gpu_fence_destroy(rawkit_gpu_t *gpu, VkFence fence);
VkResult rawkit_gpu_fence_status(rawkit_gpu_t *gpu, VkFence fence);

// SSBO
typedef struct rawkit_gpu_ssbo_t {
  RAWKIT_RESOURCE_FIELDS

  rawkit_gpu_t *gpu;
  rawkit_gpu_buffer_t *buffer;
  rawkit_gpu_buffer_t *staging_buffer;
} rawkit_gpu_ssbo_t;

// TODO: this should be rawkit_gpu_ssbo_ex
rawkit_gpu_ssbo_t *rawkit_gpu_ssbo_ex(
  rawkit_gpu_t *gpu,
  const char *name,
  uint64_t size,
  VkMemoryPropertyFlags memory_flags,
  VkBufferUsageFlags buffer_usage_flags
);

VkResult rawkit_gpu_ssbo_update(
  rawkit_gpu_ssbo_t *ssbo,
  VkQueue queue,
  VkCommandPool pool,
  void *data,
  uint64_t size
);

VkResult rawkit_gpu_ssbo_transition(
  rawkit_gpu_ssbo_t *ssbo,
  VkAccessFlags access
);

#define rawkit_gpu_ssbo(name, size) rawkit_gpu_ssbo_ex( \
  rawkit_default_gpu(), \
  name, \
  size, \
  0, \
  0 \
)

VkResult rawkit_gpu_buffer_map(
  rawkit_gpu_t *gpu,
  rawkit_gpu_buffer_t *dst,
  VkDeviceSize offset,
  VkDeviceSize size,
  void **buf
);

VkResult rawkit_gpu_buffer_unmap(
  rawkit_gpu_t *gpu,
  rawkit_gpu_buffer_t *dst,
  VkDeviceSize offset,
  VkDeviceSize size
);

#ifdef __cplusplus
  }
#endif
