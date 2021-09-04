#pragma once

#include <rawkit/gpu.h>

#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>

struct GPUCommandBuffer {
  VkCommandBuffer handle;
  VkFence fence;
  VkCommandPool pool;
};

struct GPUDeleteBufferEntry {
  // TODO: this is not robust and it will likely fail for long lived operations
  //       the idea right now is to wait for like 5 frames or something before throwing away the buffer
  //
  //       ideally, we'd wire all of this stuff up to a fence that we query (no wait) later on
  //       and refcount all of the resouces that were associated with that fence (and by effect a queue submission).
  uint32_t target_tick_idx;
  rawkit_gpu_buffer_t *buffer;
};


// thread safety for queues
struct GPUQueue {
  std::mutex queue_mutex;
  rawkit_gpu_queue_t queue;
  GPUQueue(const rawkit_gpu_queue_t &q): queue(q) {
    this->queue._state = (void*)this;
  }
};

class GPUState {
  public:
    std::vector<GPUCommandBuffer> completed_command_buffers;
    std::queue<GPUDeleteBufferEntry> completed_buffers;
    std::unordered_map<VkFence, VkResult> fences;


    std::unordered_map<u32, GPUQueue *> queues;
    u32 default_queue = 0;

    uint32_t tick_idx = 0;
};

static inline u32 countBits(u32 i) {
  i = i - ((i >> 1) & 0x55555555);        // add pairs of bits
  i = (i & 0x33333333) + ((i >> 2) & 0x33333333);  // quads
  i = (i + (i >> 4)) & 0x0F0F0F0F;        // groups of 8
  return (i * 0x01010101) >> 24;          // horizontal sum of bytes
}