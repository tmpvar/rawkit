#pragma optimize("", off)
#include <rawkit/core.h>
#include <rawkit-gpu-internal.h>
#include <rawkit/hash.h>
#include <rawkit/hot.h>

#include <string>
using namespace std;

void rawkit_gpu_populate_queue_cache(rawkit_gpu_t *gpu) {
  vkGetPhysicalDeviceQueueFamilyProperties(gpu->physical_device, &gpu->queue_count, NULL);
  gpu->queue_family_properties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * gpu->queue_count);
  if (!gpu->queue_family_properties) {
    printf("ERROR: rawkit_vulkan_find_queue_family_index failed - out of memory\n");
    return;
  }
  vkGetPhysicalDeviceQueueFamilyProperties(gpu->physical_device, &gpu->queue_count, gpu->queue_family_properties);
}

int32_t rawkit_vulkan_find_queue_family_index(rawkit_gpu_t *gpu, VkQueueFlags flags) {
  if (!gpu->queue_family_properties) {
    rawkit_gpu_populate_queue_cache(gpu);
  }

  u32 best_match = 0xFFFFFFFF;
  u32 best_bit_count = 0xFFFFFFFF;

  u32 flags_bit_count = countBits(flags);

  for (i32 i = 0; i < gpu->queue_count; i++) {
    u32 queue_flags = gpu->queue_family_properties[i].queueFlags;
    if (queue_flags & flags) {
      u32 queue_bit_count = countBits(queue_flags);
      if (queue_bit_count < best_bit_count) {
        best_bit_count = queue_bit_count;
        best_match = i;
      }
    }
  }

  if (best_match != 0xFFFFFFFF) {
    return best_match;
  }

  printf("ERROR: rawkit_vulkan_find_queue_family_index failed - could not locate queue (%i)\n", flags);
  return -1;
}

VkQueue rawkit_vulkan_find_queue(rawkit_gpu_t *gpu, VkQueueFlags flags) {
  i32 idx = rawkit_vulkan_find_queue_family_index(gpu, flags);
  if (idx < 0) {
    return VK_NULL_HANDLE;
  }
  VkQueue queue;

  vkGetDeviceQueue(gpu->device, idx, 0, &queue);
  return queue;
}

rawkit_gpu_queue_t rawkit_gpu_queue_by_family(rawkit_gpu_t *gpu, u32 family_idx, u32 queue_idx) {
  auto state = (GPUState *)gpu->_state;
  auto it = state->queues.find(family_idx);
  if (it == state->queues.end()) {
    rawkit_gpu_queue_t q = {};
    vkGetDeviceQueue(gpu->device, family_idx, queue_idx, &q.handle);
    q.family_idx = family_idx;

    // Create the associated command pool
    {
      VkCommandPoolCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      info.queueFamilyIndex = family_idx;
      VkResult err = vkCreateCommandPool(
        gpu->device,
        &info,
        gpu->allocator,
        &q.command_pool
      );

      if (err) {
        printf("ERROR: rawkit_gpu_queue: could not allocate command pool (%i)\n", err);
        return {};
      }
    }

    state->queues[family_idx] = new GPUQueue(q);
    return q;
  }

  return it->second->queue;
}
rawkit_gpu_queue_t rawkit_gpu_queue_ex(rawkit_gpu_t *gpu, VkQueueFlags flags, u32 queue_idx) {
  if (!gpu || !gpu->_state || !i32(flags)) {
    return {};
  }

  auto state = (GPUState *)gpu->_state;

  i32 family_idx = rawkit_vulkan_find_queue_family_index(gpu, flags);
  if (family_idx < 0) {
    return {};
  }

  return rawkit_gpu_queue_by_family(gpu, family_idx, queue_idx);
}

VkResult rawkit_gpu_queue_submit_ex(
  rawkit_gpu_queue_t *queue,
  VkCommandBuffer command_buffer,
  VkFence fence
) {
  // we need the root level rawkit_gpu here so all threads lock against
  // the same set of mutexes.
  auto gpu = rawkit_default_gpu();

  if (!gpu || !gpu->_state) {
    printf("ERROR: rawkit_gpu_queue_enqueue: invalid gpu\n");
    return VK_INCOMPLETE;
  }

  if (!queue || !queue->_state) {
    printf("ERROR: rawkit_gpu_queue_enqueue: invalid queue\n");
    return VK_INCOMPLETE;
  }

  if (!command_buffer) {
    printf("ERROR: rawkit_gpu_queue_enqueue: invalid command_buffer\n");
    return VK_INCOMPLETE;
  }

  auto gpu_state = (GPUState *)gpu->_state;
  auto queue_state = (GPUQueue *)queue->_state;

  {
    std::lock_guard<std::mutex> guard(queue_state->queue_mutex);

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_buffer;
    VkResult err = vkQueueSubmit(queue->handle, 1, &submit, fence);
    if (err) {
      printf("ERROR: rawkit_gpu_queue_submit_ex: unable to submit command buffer (%i)\n", err);
      return err;
    }
  }

  return VK_SUCCESS;
}

uint32_t rawkit_vulkan_find_memory_type(rawkit_gpu_t *gpu, VkMemoryPropertyFlags properties, uint32_t type_bits) {
  VkPhysicalDeviceMemoryProperties prop;
  vkGetPhysicalDeviceMemoryProperties(gpu->physical_device, &prop);
  for (uint32_t i = 0; i < prop.memoryTypeCount; i++) {
    if (
      (prop.memoryTypes[i].propertyFlags & properties) == properties &&
      type_bits & (1 << i)
    ) {
      return i;
    }
  }
  return 0xFFFFFFFF; // Unable to find memoryType
}

VkResult rawkit_gpu_buffer_destroy(rawkit_gpu_t *gpu, rawkit_gpu_buffer_t *buf) {
  if (!buf || !buf->memory || !buf->handle) {
    return VK_SUCCESS;
  }

  vkFreeMemory(gpu->device, buf->memory, NULL);
  vkDestroyBuffer(gpu->device, buf->handle, NULL);
  buf->memory = nullptr;
  buf->handle = nullptr;
  return VK_SUCCESS;
}

rawkit_gpu_buffer_t *rawkit_gpu_buffer_create(
  const char *name,
  rawkit_gpu_t *gpu,
  VkDeviceSize size,
  VkMemoryPropertyFlags memory_flags,
  VkBufferUsageFlags buffer_usage_flags
) {
  if (!gpu) {
    return NULL;
  }

  rawkit_gpu_buffer_t *buf = rawkit_hot_resource(name, rawkit_gpu_buffer_t);
  if (!buf) {
    printf("ERROR: rawkit_gpu_buffer_create: unable to allocate rawkit_gpu_buffer\n");
    return NULL;
  }

  bool dirty = (
    buf->resource_version == 0 ||
    buf->buffer_usage_flags != buffer_usage_flags ||
    buf->memory_flags != memory_flags ||
    buf->size != size
  );

  if (!dirty) {
    return buf;
  }

  printf("Rebuilding Buffer (%llu, %s) size(%u -> %u)\n",
    buf->resource_id,
    buf->resource_name,
    buf->size,
    size
  );

  if (buf->handle) {
    rawkit_gpu_queue_buffer_for_deletion(buf);
  }

  buf->size = size;
  buf->gpu = gpu;
  buf->memory_flags = memory_flags,
  buf->buffer_usage_flags = buffer_usage_flags;
  buf->handle = nullptr;
  buf->memory = nullptr;
  VkResult err;

  // create the buffer
  {
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = buffer_usage_flags;

    err = vkCreateBuffer(
      gpu->device,
      &info,
      gpu->allocator,
      &buf->handle
    );

    if (err) {
      printf("ERROR: rawkit_gpu_buffer_create: unable to create buffer (%i)\n", err);
      return NULL;
    }
  }

  // allocate device memory
  VkMemoryRequirements req;
  {
    vkGetBufferMemoryRequirements(
      gpu->device,
      buf->handle,
      &req
    );

    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = req.size;
    info.memoryTypeIndex = rawkit_vulkan_find_memory_type(
      gpu,
      memory_flags,
      req.memoryTypeBits
    );

    err = vkAllocateMemory(
      gpu->device,
      &info,
      gpu->allocator,
      &buf->memory
    );

    if (err) {
      printf("ERROR: rawkit_gpu_buffer_create: unable to allocate memory for buffer (bytes=%llu) (err=%i)\n", size, err);
      return NULL;
    }
  }

  // bind buffer to memory
  {
    err = vkBindBufferMemory(
      gpu->device,
      buf->handle,
      buf->memory,
      0
    );

    if (err) {
      printf("ERROR: rawkit_gpu_buffer_create: unable to bind memory for buffer (%i)\n", err);
      return NULL;
    }
  }

  buf->resource_version++;

  return buf;
}

VkResult rawkit_gpu_vertex_buffer_destroy(rawkit_gpu_t *gpu, rawkit_gpu_vertex_buffer_t *buf) {
  if (!buf) {
    return VK_SUCCESS;
  }

  rawkit_gpu_buffer_destroy(gpu, buf->vertices);
  buf->vertices = NULL;

  free(buf);

  return VK_SUCCESS;
}


VkFence rawkit_gpu_copy_buffer(
  rawkit_gpu_t *gpu,
  VkQueue queue,
  VkCommandPool pool,
  rawkit_gpu_buffer_t *src,
  rawkit_gpu_buffer_t *dst,
  VkDeviceSize src_offset,
  VkDeviceSize dst_offset,
  VkDeviceSize size
) {
  VkResult err;
  VkCommandBuffer command_buffer;
  // allocate a command buffer
  {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    err = vkAllocateCommandBuffers(gpu->device, &info, &command_buffer);
    if (err) {
      printf("ERROR: could not allocate command buffers while copying buffer (%i)\n", err);
      return VK_NULL_HANDLE;
    }
  }

  // begin the command buffer
  {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(command_buffer, &info);
    if (err) {
      printf("ERROR: could not begin command buffer while copying buffer (%i)\n", err);
      return VK_NULL_HANDLE;
    }
  }

  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = src_offset;
  copyRegion.dstOffset = dst_offset;
  copyRegion.size = size;

  // Vertex buffer
  vkCmdCopyBuffer(
    command_buffer,
    src->handle,
    dst->handle,
    1,
    &copyRegion
  );

  vkEndCommandBuffer(command_buffer);
  VkFence fence = VK_NULL_HANDLE;
  {
    err = rawkit_gpu_fence_create(gpu, &fence);
    if (err) {
      printf("ERROR: create fence failed while copying buffer (%i)\n", err);
      return VK_NULL_HANDLE;
    }
  }

  // submit the command buffer to the queue
  {
    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_buffer;
    err = vkQueueSubmit(queue, 1, &submit, fence);
    if (err) {
      printf("ERROR: unable to submit command buffer to queue while copying buffer (%i)\n", err);
      return VK_NULL_HANDLE;
    }
  }

  // schedule the buffer for deletion
  {
    rawkit_gpu_queue_command_buffer_for_deletion(
      gpu,
      command_buffer,
      fence,
      pool
    );
  }

  return fence;
}

VkResult rawkit_gpu_buffer_update(
  rawkit_gpu_buffer_t *dst,
  void *src,
  VkDeviceSize size
) {
  if (!dst || !src || size == 0) {
    return VK_INCOMPLETE;
  }

  void *ptr;
  VkResult err = vkMapMemory(
    dst->gpu->device,
    dst->memory,
    0,
    size,
    0,
    &ptr
  );

  if (err) {
    printf("ERROR: rawkit_gpu_buffer_update: unable to map memory to update buffer (%s) contents (%i)\n", dst->resource_name, err);
    return err;
  }

  memcpy(ptr, src, size);

  vkUnmapMemory(dst->gpu->device, dst->memory);
  return VK_SUCCESS;
}

rawkit_gpu_vertex_buffer_t *rawkit_gpu_vertex_buffer_create(
  rawkit_gpu_t *gpu,
  VkQueue queue,
  VkCommandPool pool,
  rawkit_mesh_t *mesh
) {
  if (!gpu || !mesh) {
    return NULL;
  }
  const char *resource_name = "rawkit::gpu::vertex-buffer";
  uint64_t id = rawkit_hash_resources(resource_name, 1, (const rawkit_resource_t **)&mesh);
  rawkit_gpu_vertex_buffer_t *vb = rawkit_hot_resource_id(resource_name, id, rawkit_gpu_vertex_buffer_t);
  bool dirty = rawkit_resource_sources(vb, mesh);

  if (!dirty) {
    return vb;
  }

  uint32_t vertex_count = rawkit_mesh_vertex_count(mesh);

  if (!vb || !vertex_count || !mesh->vertex_data) {
    return NULL;
  }


  VkDeviceSize vertices_size = vertex_count * 3 * sizeof(float);
  VkResult err;

  // setup vertices
  rawkit_gpu_buffer_t *vertices = NULL;
  {
    string buffer_name = string(resource_name) + "-vertex-buffer";
    vertices = rawkit_gpu_buffer_create(
      buffer_name.c_str(),
      gpu,
      vertices_size,
      (
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ),
      (
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
      )
    );
    string staging_buffer_name = string(resource_name) + "-vertex-buffer-staging";
    rawkit_gpu_buffer_t *vertices_staging = rawkit_gpu_buffer_create(
      staging_buffer_name.c_str(),
      gpu,
      vertices_size,
      (
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      ),
      (
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT
      )
    );

    err = rawkit_gpu_buffer_update(
      vertices_staging,
      mesh->vertex_data,
      vertices_size
    );
    if (err) {
      printf("ERROR: unable to upload new vertex data to gpu buffer\n");
      free(vb);
      rawkit_gpu_buffer_destroy(gpu, vertices_staging);
      return NULL;
    }

    rawkit_gpu_copy_buffer(
      gpu,
      queue,
      pool,
      vertices_staging,
      vertices,
      0,
      0,
      vertices_size
    );

    // TODO: schedule a deletion
    // rawkit_gpu_buffer_destroy(gpu, vertices_staging);

    if (err) {
      printf("ERROR: unable to copy buffer `vertices_staging` to `vb->vertices`\n");
      free(vb);
      return NULL;
    }
  }


  // setup the index buffer
  uint32_t index_count = rawkit_mesh_index_count(mesh);
  VkDeviceSize indices_size = index_count * sizeof(uint32_t);
  rawkit_gpu_buffer_t *indices = NULL;
  if (index_count && mesh->index_data) {
    string buffer_name = string(resource_name) + "-index-buffer";
    indices = rawkit_gpu_buffer_create(
      buffer_name.c_str(),
      gpu,
      indices_size,
      (
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ),
      (
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT
      )
    );

    string staging_buffer_name = string(resource_name) + "-index-buffer-staging";
    rawkit_gpu_buffer_t *staging = rawkit_gpu_buffer_create(
      staging_buffer_name.c_str(),
      gpu,
      indices_size,
      (
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      ),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    );

    err = rawkit_gpu_buffer_update(
      staging,
      mesh->index_data,
      indices_size
    );
    if (err) {
      printf("ERROR: unable to upload new vertex data to gpu buffer\n");

      rawkit_gpu_buffer_destroy(gpu, staging);
      rawkit_gpu_buffer_destroy(gpu, vb->indices);
      return NULL;
    }

    rawkit_gpu_copy_buffer(
      gpu,
      queue,
      pool,
      staging,
      indices,
      0,
      0,
      indices_size
    );

    if (err) {
      printf("ERROR: unable to copy buffer `indices_staging` to `ib->indices`\n");

      rawkit_gpu_buffer_destroy(gpu, vb->indices);
      rawkit_gpu_buffer_destroy(gpu, staging);
      return NULL;
    }
    // TODO: schedule a deletion
    // rawkit_gpu_buffer_destroy(gpu, staging);
  }

  if (vb->vertices) {
    rawkit_gpu_buffer_destroy(gpu, vb->vertices);
  }
  vb->vertices = vertices;

  if (vb->indices) {
    rawkit_gpu_buffer_destroy(gpu, vb->indices);
  }
  vb->indices = indices;

  vb->resource_version++;

  return vb;
}

rawkit_gpu_queue_t rawkit_gpu_default_queue_ex(rawkit_gpu_t *gpu) {
  if (!gpu || !gpu->_state) {
    return {};
  }

  auto state = (GPUState *)gpu->_state;

  return rawkit_gpu_queue_by_family(gpu, state->default_queue, 0);
}

VkCommandPool rawkit_gpu_default_command_pool_ex(rawkit_gpu_t *gpu) {
  auto queue = rawkit_gpu_default_queue_ex(gpu);
  return queue.command_pool;
}

VkCommandPool rawkit_gpu_command_pool_ex(rawkit_gpu_t *gpu, VkQueueFlags flags) {
  if (!gpu || !gpu->_state || !flags) {
    printf("ERROR: rawkit_gpu_command_pool_ex: invalid arguments\n");
    return VK_NULL_HANDLE;
  }

  auto queue = rawkit_gpu_queue_ex(gpu, flags, 0);
  return queue.command_pool;
}

VkCommandBuffer rawkit_gpu_create_command_buffer(rawkit_gpu_t *gpu, VkCommandPool pool) {
  if (!gpu || !pool) {
    printf("ERROR: rawkit_gpu_create_command_buffer: invalid arguments\n");
    return VK_NULL_HANDLE;
  }
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = pool == VK_NULL_HANDLE ? gpu->command_pool : pool;
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = 1;
  VkCommandBuffer command_buffer;
  VkResult err = vkAllocateCommandBuffers(
    gpu->device,
    &info,
    &command_buffer
  );

  if (err) {
    printf("ERROR: failed to allocate command buffers (%i)\n", err);
    return VK_NULL_HANDLE;
  }

  return command_buffer;
}



VkResult rawkit_gpu_fence_create(rawkit_gpu_t *gpu, VkFence *fence) {
  if (!gpu || !gpu->_state) {
    return VK_INCOMPLETE;
  }

  GPUState *state = (GPUState *)gpu->_state;

  VkFenceCreateInfo create = {};
  create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  create.flags = 0;
  VkResult err = vkCreateFence(gpu->device, &create, gpu->allocator, fence);
  if (err) {
    return err;
  }

  state->fences[*fence] = VK_NOT_READY;
  return VK_SUCCESS;
}

void rawkit_gpu_fence_destroy(rawkit_gpu_t *gpu, VkFence fence) {
  if (!gpu || !gpu->_state) {
    return;
  }

  GPUState *state = (GPUState *)gpu->_state;

  auto it = state->fences.find(fence);
  if (it != state->fences.end()) {
    state->fences.erase(it);
    vkDestroyFence(
      gpu->device,
      fence,
      gpu->allocator
    );
  } else {
    printf("ERROR: rawkit_gpu_fence_destroy: could not find reference to fence(%i)\n", fence);
  }
}


VkResult rawkit_gpu_fence_status(rawkit_gpu_t *gpu, VkFence fence) {
  if (!gpu || !gpu->_state) {
    return VK_INCOMPLETE;
  }

  GPUState *state = (GPUState *)gpu->_state;

  auto fence_it = state->fences.find(fence);
  if (fence_it != state->fences.end()) {
    VkResult fence_status = vkGetFenceStatus(gpu->device, fence);
    fence_it->second = fence_status;
    return fence_status;
  }
  return VK_SUCCESS;
}

void rawkit_gpu_tick(rawkit_gpu_t *gpu) {
  if (!gpu || !gpu->_state) {
    return;
  }

  GPUState *state = (GPUState *)gpu->_state;
  std::vector<GPUCommandBuffer> buffers;

  auto it = state->completed_command_buffers.begin();
  while(it != state->completed_command_buffers.end()) {
    bool removed = false;
    if (rawkit_gpu_fence_status(gpu, it->fence) == VK_SUCCESS) {
      if (it->handle != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(
          gpu->device,
          it->pool,
          1,
          &it->handle
        );

        it->handle = VK_NULL_HANDLE;
      }
      rawkit_gpu_fence_destroy(gpu, it->fence);
      state->completed_command_buffers.erase(it);
      removed = true;
    }

    if (!removed) {
      it++;
    }
  }

  while (state->completed_buffers.size()) {
    auto e = state->completed_buffers.front();
    if (e.target_tick_idx < state->tick_idx) {
      break;
    }

    rawkit_gpu_buffer_destroy(gpu, e.buffer);
    state->completed_buffers.pop();
  }
  state->tick_idx++;
}

void rawkit_gpu_queue_command_buffer_for_deletion(rawkit_gpu_t *gpu, VkCommandBuffer buffer, VkFence fence, VkCommandPool pool) {
  if (!gpu || !gpu->_state || !buffer || !fence) {
    return;
  }

  GPUState *state = (GPUState *)gpu->_state;
  GPUCommandBuffer b = {};
  b.handle = buffer;
  b.fence = fence;
  b.pool = pool;
  state->completed_command_buffers.push_back(b);
}

void rawkit_gpu_queue_buffer_for_deletion(rawkit_gpu_buffer_t *buffer) {
  if (!buffer->gpu || !buffer->gpu->_state) {
    return;
  }

  GPUState *state = (GPUState *)buffer->gpu->_state;
  GPUDeleteBufferEntry e = {};
  e.buffer = buffer;
  e.target_tick_idx = state->tick_idx + 5;
  state->completed_buffers.push(e);
}


uint32_t rawkit_gpu_get_tick_idx(rawkit_gpu_t *gpu) {
  if (!gpu || !gpu->_state) {
    return 0;
  }
  GPUState *state = (GPUState *)gpu->_state;
  return state->tick_idx;
}
#pragma optimize("", off)

rawkit_gpu_ssbo_t *rawkit_gpu_ssbo_ex(
  rawkit_gpu_t *gpu,
  const char *name,
  uint64_t size,
  VkMemoryPropertyFlags memory_flags,
  VkBufferUsageFlags buffer_usage_flags
) {
  std::string resource_name = std::string("rawkit-gpu-ssbo://") + name;
  rawkit_gpu_ssbo_t *ssbo = rawkit_hot_resource(resource_name.c_str(), rawkit_gpu_ssbo_t);
  ssbo->gpu = gpu;

  bool dirty = (
    ssbo->buffer == NULL ||
    ssbo->buffer->size != size
  );

  if (dirty) {
    printf("Rebuilding SSBO (%llu, %s) size(%u -> %u)\n",
      ssbo->resource_id,
      ssbo->resource_name,
      ssbo->buffer ? ssbo->buffer->size : 0,
      size
    );
    ssbo->resource_version = 0;
    if (ssbo->buffer) {
      rawkit_gpu_queue_buffer_for_deletion(ssbo->buffer);
      ssbo->buffer = nullptr;
    }

    if (ssbo->staging_buffer) {
      rawkit_gpu_queue_buffer_for_deletion(ssbo->staging_buffer);
      ssbo->staging_buffer = nullptr;
    }

    std::string buffer_name = resource_name + "/buffer";

    ssbo->buffer = rawkit_gpu_buffer_create(
      buffer_name.c_str(),
      gpu,
      size,
      (
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        memory_flags
      ),
      (
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        buffer_usage_flags
      )
    );

    std::string staging_buffer_name = resource_name + "/staging-buffer";
    ssbo->staging_buffer = rawkit_gpu_buffer_create(
      staging_buffer_name.c_str(),
      gpu,
      size,
      (
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      ),
      (
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT
      )
    );

    ssbo->resource_version++;
  }

  return ssbo;
}


VkResult rawkit_gpu_ssbo_transition(
  rawkit_gpu_ssbo_t *ssbo,
  VkAccessFlags access
) {
  if (!ssbo || !ssbo->gpu || !ssbo->buffer) {
    return VK_INCOMPLETE;
  }

  rawkit_gpu_t *gpu = ssbo->gpu;

  VkResult err = VK_SUCCESS;
  VkCommandBuffer command_buffer = rawkit_gpu_create_command_buffer(gpu, nullptr);
  if (!command_buffer) {
    printf("ERROR: rawkit_gpu_ssbo_ex: could not create command buffer\n");
    return err;
  }
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VkResult begin_result = vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferMemoryBarrier barrier = {};
  barrier.dstAccessMask = access;

  rawkit_gpu_buffer_transition(
    ssbo->buffer,
    command_buffer,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    barrier
  );

  {
    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;
    err = vkEndCommandBuffer(command_buffer);
    if (err) {
      printf("ERROR: rawkit_gpu_ssbo_ex: could not end command buffer");
      return err;
    }

    VkFence fence;
    {
      err = rawkit_gpu_fence_create(gpu, &fence);
      if (err) {
        printf("ERROR: rawkit_gpu_ssbo_ex: create fence failed (%i)\n", err);
        return err;
      }
    }

    err = vkQueueSubmit(gpu->graphics_queue, 1, &end_info, fence);
    rawkit_gpu_queue_command_buffer_for_deletion(gpu, command_buffer, fence, gpu->command_pool);
    if (err) {
      printf("ERROR: rawkit_gpu_ssbo_ex: could not submit command buffer");
      return err;
    }
  }

  return VK_SUCCESS;
}

VkResult rawkit_gpu_ssbo_update(
  rawkit_gpu_ssbo_t *ssbo,
  VkQueue queue,
  VkCommandPool pool,
  void *data,
  uint64_t size
) {
  if (!ssbo || !data || !size) {
    return VK_INCOMPLETE;
  }

  if (!ssbo->gpu || !ssbo->buffer || !ssbo->staging_buffer) {
    return VK_INCOMPLETE;
  }

  VkResult err;

  err = rawkit_gpu_buffer_update(
    ssbo->staging_buffer,
    data,
    size
  );

  if (err) {
    printf("ERROR: rawkit_gpu_ssbo_update(%s): could not update staging buffer (%i)\n", ssbo->resource_name, err);
    return err;
  }

  rawkit_gpu_copy_buffer(
    ssbo->gpu,
    queue,
    pool,
    ssbo->staging_buffer,
    ssbo->buffer,
    0,
    0,
    ssbo->buffer->size < size ? ssbo->buffer->size : size
  );

  if (err) {
    printf("ERROR: rawkit_gpu_ssbo_update(%s): could not copy staging buffer to gpu buffer (%i)\n", ssbo->resource_name, err);
    return err;
  }

  ssbo->resource_version++;

  return VK_SUCCESS;
}

VkResult rawkit_gpu_buffer_transition(
  rawkit_gpu_buffer_t *buffer,
  VkCommandBuffer command_buffer,
  VkPipelineStageFlags src_stage,
  VkPipelineStageFlags dest_stage,
  VkBufferMemoryBarrier extend
) {
  if (!buffer || !command_buffer) {
    return VK_INCOMPLETE;
  }

  VkBufferMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

  // FIXME: when transitioning between transfer/host r+w things appear to be out of sync
  //        and a validation error is thrown.
  // barrier.srcAccessMask = RAWKIT_DEFAULT(extend.srcAccessMask, buffer->access);
  barrier.dstAccessMask = RAWKIT_DEFAULT(extend.dstAccessMask, VK_ACCESS_SHADER_READ_BIT);
  barrier.size = VK_WHOLE_SIZE;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = buffer->handle;

  if (buffer->access == barrier.dstAccessMask) {
    return VK_SUCCESS;
  }

  vkCmdPipelineBarrier(
    command_buffer,
    src_stage,
    dest_stage,
    0,
    0,
    NULL,
    1,
    &barrier,
    0,
    NULL
  );
  buffer->access = barrier.dstAccessMask;
  return VK_SUCCESS;
}

VkResult rawkit_gpu_buffer_map(rawkit_gpu_t *gpu, rawkit_gpu_buffer_t *dst, VkDeviceSize offset, VkDeviceSize size, void **buf) {
  if (size >= dst->size) {
    size = VK_WHOLE_SIZE;
  }

  VkResult err = vkMapMemory(
    gpu->device,
    dst->memory,
    offset,
    size,
    0,
    buf
  );

  if (err) {
    printf("ERROR: unable to map memory (%i)\n", err);
    return err;
  }

  return err;
}

VkResult rawkit_gpu_buffer_unmap(rawkit_gpu_t *gpu, rawkit_gpu_buffer_t *dst, VkDeviceSize offset, VkDeviceSize size) {
  VkResult err = VK_SUCCESS;
  // flush
  {
    VkMappedMemoryRange flush = {};
    flush.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    flush.memory = dst->memory;
    flush.offset = offset;
    flush.size = size;

    VkResult err = vkFlushMappedMemoryRanges(
      gpu->device,
      1,
      &flush
    );

    if (err) {
      printf("ERROR: unable to flush mapped memory ranges (%i)\n", err);
    }
  }

  vkUnmapMemory(gpu->device, dst->memory);
  return err;
}