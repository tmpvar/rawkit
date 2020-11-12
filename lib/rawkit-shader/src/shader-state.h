#pragma once

#include <stdint.h>

#include <rawkit/glsl.h>
#include <rawkit/gpu.h>
#include <rawkit/vulkan.h>

#include <rawkit/shader.h>

#include <vector>
#include <array>
#include <unordered_map>
#include <string>
using namespace std;

class ConcurrentStateEntry {
  public:
    ConcurrentStateEntry();
    ~ConcurrentStateEntry();
    vector<VkDescriptorSet> descriptor_sets;
    VkCommandBuffer command_buffer;
    unordered_map<string, rawkit_gpu_buffer_t *> buffers;
    rawkit_gpu_t *gpu = nullptr;
};

class ShaderState {
  public:
    const rawkit_glsl_t *glsl = nullptr;
    rawkit_gpu_t *gpu = nullptr;

    vector<VkWriteDescriptorSet> writes;
    vector<VkShaderModule> modules;

    vector<ConcurrentStateEntry *> concurrent_entries;

    VkDescriptorPool descriptor_pool;
    VkCommandPool command_pool = VK_NULL_HANDLE;

    vector<VkDescriptorSetLayout> descriptor_set_layouts;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;



    static ShaderState *create(rawkit_gpu_t *gpu, const rawkit_glsl_t *glsl, uint8_t concurrency, VkRenderPass render_pass);
    ~ShaderState();

    bool valid();

    VkResult create_pipeline_layout();
    VkResult populate_concurrent_entries();
    VkResult create_graphics_pipeline(VkRenderPass render_pass);
    VkResult create_compute_pipeline();
};