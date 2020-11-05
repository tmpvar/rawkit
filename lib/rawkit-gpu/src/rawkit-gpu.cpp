#include <rawkit/gpu.h>



uint32_t rawkit_vulkan_find_memory_type(VkPhysicalDevice physical_device, VkMemoryPropertyFlags properties, uint32_t type_bits) {
  VkPhysicalDeviceMemoryProperties prop;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &prop);
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

VkResult rawkit_gpu_buffer_destroy(VkDevice device, rawkit_gpu_buffer_t *buf) {
  vkFreeMemory(device, buf->memory, NULL);
  vkDestroyBuffer(device, buf->handle, NULL);
  return VK_SUCCESS;
}

rawkit_gpu_buffer_t *rawkit_gpu_buffer_create(
  VkPhysicalDevice physical_device,
  VkDevice device,
  VkDeviceSize size,
  VkMemoryPropertyFlags flags
) {
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
    info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    err = vkCreateBuffer(
      device,
      &info,
      NULL,
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
      device,
      buf->handle,
      &req
    );

    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = req.size;
    info.memoryTypeIndex = rawkit_vulkan_find_memory_type(
      physical_device,
      flags,
      req.memoryTypeBits
    );

    err = vkAllocateMemory(
      device,
      &info,
      NULL,
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
      device,
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

VkResult rawkit_gpu_vertex_buffer_destroy(VkDevice device, rawkit_gpu_vertex_buffer_t *buf) {
  if (!buf) {
    return VK_SUCCESS;
  }

  rawkit_gpu_buffer_destroy(device, buf->vertices);
  buf->vertices = NULL;

  free(buf);

  return VK_SUCCESS;
}

rawkit_gpu_vertex_buffer_t *rawkit_gpu_vertex_buffer_create(
  VkPhysicalDevice physical_device,
  VkDevice device,
  VkQueue queue,
  VkCommandPool pool,
  uint32_t vertex_count,
  float *vertices
) {
  rawkit_gpu_vertex_buffer_t *vb = (rawkit_gpu_vertex_buffer_t *)calloc(
    sizeof(rawkit_gpu_vertex_buffer_t),
    1
  );

  if (!vb) {
    return NULL;
  }

  VkDeviceSize size = vertex_count * 3 * sizeof(float);
  VkResult err;

  vb->vertices = rawkit_gpu_buffer_create(
    physical_device,
    device,
    size,
    (
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT
    )
  );
  rawkit_gpu_buffer_t *vertices_staging = rawkit_gpu_buffer_create(
    physical_device,
    device,
    size,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT
  );

  // map the buffer and copy over the vertex data
  {
    void *ptr;
    err = vkMapMemory(
      device,
      vertices_staging->memory,
      0,
      size,
      0,
      &ptr
    );

    if (err) {
      printf("ERROR: unable to map memory to set vertex buffer contents (%i)\n", err);
    }

    memcpy(ptr, vertices, size);

    vkUnmapMemory(device, vertices_staging->memory);
  }

  // from the staging buffer to the device local buffer
  {
    VkCommandBuffer command_buffer;
    // allocate a command buffer
    {
      VkCommandBufferAllocateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      info.commandPool = pool;
      info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      info.commandBufferCount = 1;

      err = vkAllocateCommandBuffers(device, &info, &command_buffer);
      if (err) {
        printf("ERROR: could not allocate command buffers while setting up a vertex buffer (%i)\n", err);
        return NULL;
      }
    }

    // begin the command buffer
    {
      VkCommandBufferBeginInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      err = vkBeginCommandBuffer(command_buffer, &info);
      if (err) {
        printf("ERROR: could not begin command buffer while setting up vertex buffer (%i)\n", err);
      }
    }


    VkBufferCopy copyRegion = {};
    copyRegion.size = size;

    // Vertex buffer
    vkCmdCopyBuffer(
      command_buffer,
      vertices_staging->handle,
      vb->vertices->handle,
      1,
      &copyRegion
    );

    vkEndCommandBuffer(command_buffer);
    VkFence fence;
    {
      VkFenceCreateInfo create = {};
      create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      create.flags = 0;
      err = vkCreateFence(device, &create, NULL, &fence);
      if (err) {
        printf("ERROR: create fence failed while setting up vertex buffer (%i)\n", err);
        return NULL;
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
        return NULL;
      }

      uint64_t timeout_ns = 100000000000;
      err = vkWaitForFences(device, 1, &fence, VK_TRUE, timeout_ns);
      if (err) {
        printf("ERROR: unable to wait for queue while setting up vertex buffer (%i)\n", err);
        return NULL;
      }

      vkDestroyFence(device, fence, NULL);
      vkFreeCommandBuffers(device, pool, 1, &command_buffer);
    }
  }

  rawkit_gpu_buffer_destroy(device, vertices_staging);

  return vb;
}
