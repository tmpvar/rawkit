#pragma once

#include <vector>
#include <string>
#include <functional>
using namespace std;

char buffer_tmp_str[4096];

static VkMemoryPropertyFlags default_memory_flags = (
  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
);

static VkBufferUsageFlags default_buffer_usage_flags = (
  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
  VK_BUFFER_USAGE_TRANSFER_SRC_BIT
);

template <typename T>
struct Buffer {
  rawkit_gpu_buffer_t *_buffer;
  const u32 length;
  Buffer(const char *name, u32 length)
    : Buffer(
        name,
        length,
        nullptr,
        default_memory_flags,
        default_buffer_usage_flags
      )
  {
  }

  Buffer(
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

    sprintf(buffer_tmp_str, "FrameGraph-%s-", name);
    u32 size = length * sizeof(T);
    printf("Create gpu buffer with %u elements (%u bytes)", length, size);
    this->_buffer = rawkit_gpu_buffer_create(
      gpu,
      size,
      memory_flags,
      buffer_usage_flags
    );
  }

  rawkit_gpu_buffer_t *handle() {
    return this->_buffer;
  }

  void write(vector<T> values, u32 offset = 0) {
    if (offset >= length) {
      return;
    }

    if (this->_buffer->buffer_usage_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      rawkit_gpu_buffer_update(
        this->_buffer,
        (void *)values.data(),
        values.size() * sizeof(T)
      );
      return;
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

  T *readOne(u32 index) {
    // if (index >= length) {
    //   return nullptr;
    // }

    // return &this->slab[index];
    return nullptr;
  }

  VkResult map(std::function<void(T *)> cb) {
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
};