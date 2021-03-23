#pragma once

#include <rawkit/gpu.h>

#include <vector>
#include <queue>

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
  rawkit_gpu_buffer_t buffer;
};

class GPUState {
  public:
    std::vector<GPUCommandBuffer> completed_command_buffers;
    std::queue<GPUDeleteBufferEntry> completed_buffers;
    uint32_t tick_idx = 0;
};