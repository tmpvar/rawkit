#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rawkit/rawkit.h>

#include <rawkit/mesh.h>

typedef struct rawkit_gpu_buffer_t {
  VkDeviceMemory memory;
  VkBuffer handle;
} rawkit_gpu_buffer_t;

typedef struct rawkit_gpu_vertex_buffer_t {
  rawkit_gpu_buffer_t *vertices;
} rawkit_gpu_vertex_buffer_t;

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

rawkit_gpu_buffer_t *rawkit_gpu_buffer_create(VkDevice device, VkDeviceSize size, VkMemoryPropertyFlags flags) {
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
      rawkit_vulkan_physical_device(),
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

rawkit_gpu_vertex_buffer_t *rawkit_gpu_vertex_buffer_create(uint32_t vertex_count, float *vertices) {
  rawkit_gpu_vertex_buffer_t *vb = (rawkit_gpu_vertex_buffer_t *)calloc(
    sizeof(rawkit_gpu_vertex_buffer_t),
    1
  );

  if (!vb) {
    return NULL;
  }

  VkDevice device = rawkit_vulkan_device();
  VkQueue queue = rawkit_vulkan_queue();

  VkDeviceSize size = vertex_count * 3 * sizeof(float);
  VkResult err;

  vb->vertices = rawkit_gpu_buffer_create(
    device,
    size,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
    VK_BUFFER_USAGE_TRANSFER_DST_BIT
  );
  rawkit_gpu_buffer_t *vertices_staging = rawkit_gpu_buffer_create(
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


    VkCommandPool pool = rawkit_vulkan_command_pool();
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

  printf("made it all the way here..\n");

  return vb;
}


typedef struct render_mesh_state_t {
  rawkit_shader_t *shaders;
  rawkit_glsl_t *glsl;
  uint32_t source_count;
  rawkit_glsl_source_t sources[RAWKIT_GLSL_STAGE_COUNT];

  rawkit_mesh_t *mesh;
  rawkit_gpu_vertex_buffer_t *vertex_buffer;
} render_mesh_state_t;

VkResult build_shaders(render_mesh_state_t *state, rawkit_glsl_t *glsl) {
  if (!state) {
    return VK_INCOMPLETE;
  }

  // TODO: this can change between frames
  uint32_t fb_count = rawkit_window_frame_count();

  if (state->shaders == NULL) {
    state->shaders = (rawkit_shader_t *)calloc(
      fb_count * sizeof(rawkit_shader_t),
      1
    );
  }

  for (uint32_t idx=0; idx < fb_count; idx++) {
    state->shaders[idx].physical_device = rawkit_vulkan_physical_device();
    VkResult err = rawkit_shader_init(glsl, &state->shaders[idx]);

    if (err != VK_SUCCESS) {
      printf("ERROR: rawkit_shader_init failed (%i)\n", err);
      return err;
    }
  }

  return VK_SUCCESS;
}

void render_mesh_file(const char *mesh_file, uint8_t source_count, const char **source_files) {
  if (source_count > RAWKIT_GLSL_STAGE_COUNT) {
    printf("ERROR: source count greater than the number of stages\n");
    return;
  }

  VkDevice device = rawkit_vulkan_device();
  if (device == VK_NULL_HANDLE) {
    printf("invalid vulkan device\n");
    return;
  }

  VkResult err;
  // TODO: generate a better id
  char id[4096] = "rawkit::render_mesh ";
  strcat(id, mesh_file);
  strcat(id, " ");
  for (uint8_t i=0; i<source_count; i++) {
    strcat(id, source_files[i]);
    strcat(id, " ");
  }

  render_mesh_state_t *state = rawkit_hot_state(id, render_mesh_state_t);
  if (!state) {
    printf("no state\n");
    return;
  }

  // rebuild the pipeline if any of the shaders changed
  {
    rawkit_glsl_source_t sources[RAWKIT_GLSL_STAGE_COUNT];

    bool changed = false;
    bool ready = true;
    for (uint8_t i=0; i<source_count; i++) {
      const rawkit_file_t *file = rawkit_file(source_files[i]);

      if (!file) {
        if (!state->sources[i].data) {
          ready = false;
          continue;
        }

        sources[i] = state->sources[i];
        continue;
      }
      sources[i].filename = source_files[i];
      sources[i].data = (const char *)file->data;

      printf("CHANGED: %s\n", source_files[i]);
      changed = true;
    }

    if (!ready) {
      return;
    }

    if (changed) {
      rawkit_glsl_t *glsl = rawkit_glsl_compile(
        source_count,
        sources,
        NULL
      );

      if (!rawkit_glsl_valid(glsl)) {
        return;
      }

      memcpy(state->sources, sources, sizeof(sources));

      rawkit_glsl_destroy(state->glsl);
      state->glsl = glsl;

      err = build_shaders(state, glsl);
      if (err) {
        printf("ERROR: could not build shaders\n");
        return;
      }
    }
  }

  if (!state->shaders) {
    return;
  }

  // rebuild the vertex buffer if the mesh changed
  rawkit_mesh_t *mesh = rawkit_mesh(mesh_file);
  if (mesh) {

    printf("mesh changed!\n");
    // TODO: more optimally, reuse the existing buffer if possible
    rawkit_gpu_vertex_buffer_t *vb = rawkit_gpu_vertex_buffer_create(
      rawkit_mesh_vertex_count(mesh),
      mesh->vertex_data
    );

    if (!vb) {
      printf("ERROR: could not create vertex buffer\n");
      return;
    }

    if (state->vertex_buffer) {
      rawkit_gpu_vertex_buffer_destroy(device, state->vertex_buffer);
    }

    state->vertex_buffer = vb;
  }

  // ensure we have a vertex buffer
  if (!state->vertex_buffer) {
    return;
  }

  // render the mesh
  {
    rawkit_shader_t *shader = &state->shaders[rawkit_window_frame_index()];
    VkCommandBuffer command_buffer = rawkit_vulkan_command_buffer();
    if (!command_buffer) {
      return;
    }

    VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(
      command_buffer,
      0,
      1,
      &state->vertex_buffer->vertices->handle,
      offsets
    );

    vkCmdBindPipeline(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      shader->pipeline
    );

    vkCmdDraw(command_buffer, 3, 1, 0, 0);
  }
}

void setup() {}

void loop() {
  const char *sources[] = {
    "mesh.vert",
    "mesh.frag"
  };

  render_mesh_file(
    "cube.stl",
    2,
    sources
  );
}