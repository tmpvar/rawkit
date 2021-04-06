#pragma once
#include <rawkit/rawkit.h>

#include <string>
using namespace std;

static u32 next_power_of_two(u32 n) {
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}

struct RingBufferState {
  RAWKIT_RESOURCE_FIELDS
  u32 size = 0;
  u32 write_offset = 0;
  u32 read_offset = 0;

  rawkit_gpu_buffer_t *buffer = 0;
  u8 *data = 0;
};

// ringbuffer 'monotonic "virtual" offset
// see: https://www.youtube.com/watch?v=mdPeXJ0eiGc
// NOTE: this is psuedocode and does not handle wrap around
//       on writes.
struct RingBuffer {
  RingBufferState *_state = nullptr;
  // required to be a power of two
  RingBuffer(const char *name, u32 size) {
    this->_state = rawkit_hot_resource(name, RingBufferState);
    if (!this->_state) {
      printf("ERROR: RingBuffer could not allocate hot resource\n");
      return;
    }

    size = next_power_of_two(size);

    if (!this->_state->resource_id || this->_state->size != size) {
      printf("Rebuilding RingBuffer(%s) %u bytes -> %u bytes\n", name, this->_state->size, size);

      if (this->_state->buffer) {
        vkUnmapMemory(
          rawkit_default_gpu()->device,
          this->_state->buffer->memory
        );

        rawkit_gpu_queue_buffer_for_deletion(*this->_state->buffer);
      }

      string buffer_name = string(name) + "-buffer";

      this->_state->size = size;
      this->_state->buffer = rawkit_gpu_buffer_create(
        "ringbuffer-gpu-buffer",
        rawkit_default_gpu(),
        this->_state->size,
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

      rawkit_gpu_buffer_map(
        rawkit_default_gpu(),
        this->_state->buffer,
        0,
        VK_WHOLE_SIZE,
        (void **)&this->_state->data
      );

      this->_state->resource_id++;
    }
  }

  u32 virt_to_physical(u32 virt_offset) {
    if (!this->_state) {
      return 0;
    }

    return virt_offset & (this->_state->size - 1);
  }

  u32 write(void *data, u32 size) {
    if (!this->_state) {
      return 0;
    }

    u32 o = this->virt_to_physical(this->_state->write_offset);
    memcpy((void *)(this->_state->data + o), data, size);
    this->_state->write_offset += size;
    return this->_state->write_offset;
  }

  // void *read(u32 virt_offset) {
  //   u32 offset = this->virt_to_physical(virt_offset);
  //   return (void *)(this->data + offset);
  // }
};