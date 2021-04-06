#pragma once

#include <rawkit/rawkit.h>
#include "ring-buffer.h"

#include <vector>
#include <functional>
using namespace std;

#include <glm/glm.hpp>

#define framegraph_tmp_str_len 4096
static char framegraph_tmp_str[framegraph_tmp_str_len + 1];

// Forward Declarations
struct Shader;

// Struct Definitions
struct FrameGraph {
  vector<Shader *>shaders;
  RingBuffer *ring_buffer;

  FrameGraph();
  ~FrameGraph();
  Shader &shader(const char *name, vector<const char *> filenames);
};

template <typename T>
struct Buffer {
  rawkit_gpu_buffer_t *_buffer;
  const u32 length;
  Buffer(const char *name, u32 length);
  Buffer(
    const char *name,
    u32 length,
    rawkit_gpu_t *gpu,
    VkMemoryPropertyFlags memory_flags,
    VkBufferUsageFlags buffer_usage_flags
  );

  rawkit_gpu_buffer_t *handle();
  void write(vector<T> values, u32 offset = 0);
  T *readOne(u32 index);
  VkResult map(std::function<void(T *)> cb);
};

struct Shader {
  rawkit_shader_t *handle = nullptr;
  vector<const rawkit_file_t *> files;
  rawkit_shader_options_t options;

  vector<std::function<void (rawkit_shader_instance_t *)>> io;

  ~Shader();
  Shader(const char *name, const vector<const char *> &filenames);
  template <typename T>
  Shader &buffer(const char *name, Buffer<T> &buffer);
  void rebuild();
  void dispatch(const glm::uvec3 &dims);
};

// FrameGraph Implementation
FrameGraph::FrameGraph() {
  this->ring_buffer = new RingBuffer("FrameGraph::ring_buffer", 64 * 1024 * 1024);
}

FrameGraph::~FrameGraph() {
  delete this->ring_buffer;
}

Shader &FrameGraph::shader(const char *name, vector<const char *> filenames) {
  Shader *s = new Shader(name, filenames);
  this->shaders.push_back(s);
  return *s;
}

// Shader Implementation
Shader::~Shader() {
  this->files.clear();
}

Shader::Shader(const char *name, const vector<const char *> &filenames) {
  this->options = rawkit_shader_default_options();

  for (const char *filename : filenames) {
    this->files.push_back(
      rawkit_file_relative_to(filename, RAWKIT_ENTRY_DIRNAME)
    );
  }
}

template <typename T>
Shader &Shader::buffer(const char *name, Buffer<T> &buffer) {
  // TODO: register this with the FrameGraph
  this->io.push_back([name, &buffer, this](rawkit_shader_instance_t *inst) {
    // TODO: this is where we'd figure out if we need to add a barrier and
    //       add it to a queue or something
    rawkit_shader_instance_param_buffer(inst, name, buffer.handle());
  });
  return *this;
}

void Shader::rebuild() {
  this->handle = rawkit_shader_ex(
      rawkit_default_gpu(),
      rawkit_vulkan_renderpass(),
      nullptr,
      files.size(),
      files.data()
  );
}

void Shader::dispatch(const glm::uvec3 &dims) {

  this->rebuild();

  rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
    rawkit_default_gpu(),
    this->handle,
    NULL,
    0
  );

  if (inst) {
    for (auto entry : this->io) {
      entry(inst);
    }

    rawkit_shader_instance_dispatch_compute(
      inst,
      dims.x,
      dims.y,
      dims.z
    );

    rawkit_shader_instance_end(inst);
  }
}

// Buffer Implementation
const static VkMemoryPropertyFlags default_memory_flags = (
  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
);

const static VkBufferUsageFlags default_buffer_usage_flags = (
  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
  VK_BUFFER_USAGE_TRANSFER_SRC_BIT
);

template <typename T>
Buffer<T>::Buffer(const char *name, u32 length)
  : Buffer(
      name,
      length,
      nullptr,
      default_memory_flags,
      default_buffer_usage_flags
    )
{
}

template <typename T>
Buffer<T>::Buffer(
  const char *name,
  u32 length,
  rawkit_gpu_t *gpu,
  VkMemoryPropertyFlags memory_flags,
  VkBufferUsageFlags buffer_usage_flags
)
  : length(length)
{
  if (gpu == nullptr) {
    gpu = rawkit_default_gpu();
  }

  snprintf(framegraph_tmp_str, framegraph_tmp_str_len, "FrameGraph::Buffer::%s", name);
  u32 size = length * sizeof(T);

  this->_buffer = rawkit_gpu_buffer_create(
    framegraph_tmp_str,
    gpu,
    size,
    memory_flags,
    buffer_usage_flags
  );

  igText("buffer %s version %u", framegraph_tmp_str, this->_buffer->resource_version);

}

template <typename T>
rawkit_gpu_buffer_t *Buffer<T>::handle() {
  return this->_buffer;
}
template <typename T>
void Buffer<T>::write(vector<T> values, u32 offset) {
  if (offset >= length) {
    return;
  }

  if ((this->_buffer->memory_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
    rawkit_gpu_buffer_update(
      this->_buffer,
      (void *)values.data(),
      values.size() * sizeof(T)
    );
    return;
  } else {
    igText("FrameGraph::Buffer::write unhandled buffer memory flags %s (%i)\n",
      this->_buffer->resource_name,
      this->_buffer->memory_flags
    );
  }

  // u32 l = values.size();
  // for (u32 i=0; i<l; i++) {
  //   u32 idx = offset + i;
  //   if (idx >= this->length) {
  //     break;
  //   }
  //   this->_buffer[idx] = values[i];
  // }
}

template <typename T>
T *Buffer<T>::readOne(u32 index) {
  // if (index >= length) {
  //   return nullptr;
  // }

  // return &this->slab[index];
  return nullptr;
}

template <typename T>
VkResult Buffer<T>::map(std::function<void(T *)> cb) {
  u64 offset = 0;
  u64 size = VK_WHOLE_SIZE;

  rawkit_gpu_ssbo_t *ssbo = this->handle();

  void *ptr;
  VkResult err = vkMapMemory(
    ssbo->gpu->device,
    ssbo->buffer->memory,
    offset,
    size,
    0,
    &ptr
  );

  if (err) {
    printf("ERROR: unable to map memory (%i)\n", err);
    return err;
  }

  cb((T *)ptr);

  // flush
  {
    VkMappedMemoryRange flush = {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .memory = ssbo->buffer->memory,
      .offset = offset,
      .size = size,
    };

    err = vkFlushMappedMemoryRanges(
      ssbo->gpu->device,
      1,
      &flush
    );

    if (err) {
      printf("ERROR: unable to flush mapped memory ranges (%i)\n", err);
    }
  }

  vkUnmapMemory(
    ssbo->gpu->device,
    ssbo->buffer->memory
  );

  return err;
}
