#pragma optimize("", off)
#include <rawkit/core.h>
#include <rawkit/gpu-internal.h>
#include <rawkit/hash.h>
#include <rawkit/hot.h>


#include <string>

static rawkit_gpu_t *default_gpu = nullptr;
rawkit_gpu_t *rawkit_default_gpu() {
  return default_gpu;
}

void rawkit_set_default_gpu(rawkit_gpu_t *gpu) {
  default_gpu = gpu;
}

int32_t rawkit_vulkan_find_queue_family_index(rawkit_gpu_t *gpu, VkQueueFlags flags) {
  if (!gpu->queue_family_properties) {
    vkGetPhysicalDeviceQueueFamilyProperties(gpu->physical_device, &gpu->queue_count, NULL);
    gpu->queue_family_properties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * gpu->queue_count);
    if (!gpu->queue_family_properties) {
      printf("ERROR: rawkit_vulkan_find_queue_family_index failed - out of memory\n");
      return 0;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(gpu->physical_device, &gpu->queue_count, gpu->queue_family_properties);
  }

  for (uint32_t i = 0; i < gpu->queue_count; i++) {
    if (gpu->queue_family_properties[i].queueFlags & flags) {
      return i;
    }
  }

  printf("ERROR: rawkit_vulkan_find_queue_family_index failed - could not locate queue (%i)\n", flags);
  return -1;
}

VkQueue rawkit_vulkan_find_queue(rawkit_gpu_t *gpu, VkQueueFlags flags) {
  uint32_t idx = rawkit_vulkan_find_queue_family_index(gpu, flags);
  if (idx < 0) {
    return VK_NULL_HANDLE;
  }
  VkQueue queue;

  vkGetDeviceQueue(gpu->device, idx, 0, &queue);
  return queue;
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
  vkFreeMemory(gpu->device, buf->memory, NULL);
  vkDestroyBuffer(gpu->device, buf->handle, NULL);
  return VK_SUCCESS;
}

rawkit_gpu_buffer_t *rawkit_gpu_buffer_create(
  rawkit_gpu_t *gpu,
  VkDeviceSize size,
  VkMemoryPropertyFlags memory_flags,
  VkBufferUsageFlags buffer_usage_flags
) {
  if (!gpu) {
    return NULL;
  }

  rawkit_gpu_buffer_t *buf = (rawkit_gpu_buffer_t *)calloc(sizeof(rawkit_gpu_buffer_t), 1);
  buf->size = size;
  VkResult err;

  if (!buf) {
    printf("ERROR: rawkit_gpu_buffer_create: unable to allocate rawkit_gpu_buffer\n");
    return NULL;
  }

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


VkResult rawkit_gpu_copy_buffer(
  rawkit_gpu_t *gpu,
  VkQueue queue,
  VkCommandPool pool,
  rawkit_gpu_buffer_t *src,
  rawkit_gpu_buffer_t *dst,
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
      return err;
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
      return err;
    }
  }


  VkBufferCopy copyRegion = {};
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
    VkFenceCreateInfo create = {};
    create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create.flags = 0;
    err = vkCreateFence(gpu->device, &create, gpu->allocator, &fence);
    if (err) {
      printf("ERROR: create fence failed while copying buffer (%i)\n", err);
      return err;
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
      return err;
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

  return VK_SUCCESS;
}

VkResult rawkit_gpu_buffer_update(
  rawkit_gpu_t *gpu,
  rawkit_gpu_buffer_t *dst,
  void *src,
  VkDeviceSize size
) {
  if (!dst || !src || size == 0) {
    return VK_INCOMPLETE;
  }

  void *ptr;
  VkResult err = vkMapMemory(
    gpu->device,
    dst->memory,
    0,
    size,
    0,
    &ptr
  );

  if (err) {
    printf("ERROR: unable to map memory to set vertex buffer contents (%i)\n", err);
    return err;
  }

  memcpy(ptr, src, size);

  vkUnmapMemory(gpu->device, dst->memory);
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
    vertices = rawkit_gpu_buffer_create(
      gpu,
      vertices_size,
      (
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT
      ),
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );
    rawkit_gpu_buffer_t *vertices_staging = rawkit_gpu_buffer_create(
      gpu,
      vertices_size,
      (
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT
      ),
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

    err = rawkit_gpu_buffer_update(
      gpu,
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

    err = rawkit_gpu_copy_buffer(
      gpu,
      queue,
      pool,
      vertices_staging,
      vertices,
      vertices_size
    );

    rawkit_gpu_buffer_destroy(gpu, vertices_staging);

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
    indices = rawkit_gpu_buffer_create(
      gpu,
      indices_size,
      (
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT
      ),
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    rawkit_gpu_buffer_t *staging = rawkit_gpu_buffer_create(
      gpu,
      indices_size,
      (
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT
      ),
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    err = rawkit_gpu_buffer_update(
      gpu,
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

    err = rawkit_gpu_copy_buffer(
      gpu,
      queue,
      pool,
      staging,
      indices,
      indices_size
    );

    if (err) {
      printf("ERROR: unable to copy buffer `indices_staging` to `ib->indices`\n");

      rawkit_gpu_buffer_destroy(gpu, vb->indices);
      rawkit_gpu_buffer_destroy(gpu, staging);
      return NULL;
    }
    rawkit_gpu_buffer_destroy(gpu, staging);
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

VkCommandBuffer rawkit_gpu_create_command_buffer(rawkit_gpu_t *gpu) {
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = gpu->command_pool;
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = 1;
  VkCommandBuffer command_buffer;
  VkResult err = vkAllocateCommandBuffers(
    gpu->device,
    &info,
    &command_buffer
  );

  if (err) {
    printf("ERROR: failed to allocate command buffers\n");
    return VK_NULL_HANDLE;
  }

  return command_buffer;
}

void rawkit_gpu_tick(rawkit_gpu_t *gpu) {
  if (!gpu || !gpu->_state) {
    return;
  }

  GPUState *state = (GPUState *)gpu->_state;
  std::vector<GPUCommandBuffer> buffers;
  for (auto &buffer : state->completed_command_buffers) {
    VkResult res = vkGetFenceStatus(gpu->device, buffer.fence);
    switch (res) {
      case VK_SUCCESS:
        {
          vkFreeCommandBuffers(
            gpu->device,
            buffer.pool,
            1,
            &buffer.handle
          );

          vkDestroyFence(
            gpu->device,
            buffer.fence,
            gpu->allocator
          );
        }
        break;
      case VK_NOT_READY:
        buffers.push_back(buffer);
        break;
      case VK_ERROR_DEVICE_LOST:
        printf("ERROR: lost device %s:%u\n", __FILE__, __LINE__);
        break;
    }
  }

  while (state->completed_buffers.size()) {
    auto e = state->completed_buffers.front();
    if (e.target_tick_idx < state->tick_idx) {
      break;
    }

    rawkit_gpu_buffer_destroy(gpu, &e.buffer);
    state->completed_buffers.pop();
  }
  state->completed_command_buffers.clear();
  state->completed_command_buffers = buffers;
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


void rawkit_gpu_queue_buffer_for_deletion(rawkit_gpu_t *gpu, const rawkit_gpu_buffer_t *buffer) {
  if (!gpu || !gpu->_state || !buffer) {
    return;
  }

  GPUState *state = (GPUState *)gpu->_state;
  GPUDeleteBufferEntry e = {};
  e.buffer = *buffer;
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
    ssbo->resource_version = 0;
    if (ssbo->buffer) {
      rawkit_gpu_queue_buffer_for_deletion(gpu, ssbo->buffer);
    }

    if (ssbo->staging_buffer) {
      rawkit_gpu_queue_buffer_for_deletion(gpu, ssbo->staging_buffer);
    }

    ssbo->buffer = rawkit_gpu_buffer_create(
      gpu,
      size,
      (
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        memory_flags
      ),
      (
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        buffer_usage_flags
      )
    );

    ssbo->staging_buffer = rawkit_gpu_buffer_create(
      gpu,
      size,
      (
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      ),
      (
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT
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
  VkCommandBuffer command_buffer = rawkit_gpu_create_command_buffer(gpu);
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
      VkFenceCreateInfo create = {};
      create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      create.flags = 0;
      err = vkCreateFence(gpu->device, &create, gpu->allocator, &fence);
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
    ssbo->gpu,
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

  barrier.srcAccessMask = RAWKIT_DEFAULT(extend.srcAccessMask, buffer->access);
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