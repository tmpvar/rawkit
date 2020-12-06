#pragma once

#include <rawkit/gpu.h>

#include <vector>

struct GPUCommandBuffer {
  VkCommandBuffer handle;
  VkFence fence;
  VkCommandPool pool;
};

class GPUState {
  public:
    std::vector<GPUCommandBuffer> completed_command_buffers;
    uint32_t tick_idx = 0;
};