#include <rawkit/gpu.h>
#include <rawkit/hash.h>
#include <rawkit/hot.h>

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
  VkResult err;

  if (!buf) {
    printf("ERROR: unable to allocate rawkit_gpu_buffer\n");
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
      printf("ERROR: unable to create buffer for vertex buffer (%i)\n", err);
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
      printf("ERROR: unable to allocate memory for vertex buffer (%i)\n", err);
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
      printf("ERROR: unable to bind memory for vertex buffer (%i)\n", err);
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
      printf("ERROR: could not allocate command buffers while setting up a vertex buffer (%i)\n", err);
      return err;
    }
  }

  // begin the command buffer
  {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    err = vkBeginCommandBuffer(command_buffer, &info);
    if (err) {
      printf("ERROR: could not begin command buffer while setting up vertex buffer (%i)\n", err);
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
  VkFence fence;
  {
    VkFenceCreateInfo create = {};
    create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create.flags = 0;
    err = vkCreateFence(gpu->device, &create, NULL, &fence);
    if (err) {
      printf("ERROR: create fence failed while setting up vertex buffer (%i)\n", err);
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
      printf("ERROR: unable to submit command buffer to queue while setting up vertex buffer (%i)\n", err);
      return err;
    }

    uint64_t timeout_ns = 100000000000;
    err = vkWaitForFences(gpu->device, 1, &fence, VK_TRUE, timeout_ns);
    if (err) {
      printf("ERROR: unable to wait for queue while setting up vertex buffer (%i)\n", err);
      return err;
    }

    vkDestroyFence(gpu->device, fence, NULL);
    vkFreeCommandBuffers(gpu->device, pool, 1, &command_buffer);
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

    if (err) {
      printf("ERROR: unable to copy buffer `vertices_staging` to `vb->vertices`\n");
      free(vb);
      rawkit_gpu_buffer_destroy(gpu, vertices_staging);
      return NULL;
    }

    rawkit_gpu_buffer_destroy(gpu, vertices_staging);
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