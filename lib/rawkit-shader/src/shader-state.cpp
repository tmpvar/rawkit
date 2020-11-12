#include "shader-state.h"

bool ShaderState::valid() {
  if (!this->gpu || !this->glsl) {
    return false;
  }

  return true;
}

ShaderState *ShaderState::create(rawkit_gpu_t *gpu, const rawkit_glsl_t *glsl, uint8_t concurrency, VkRenderPass render_pass) {
  if (!gpu || !gpu->device || !gpu->physical_device || !glsl || !concurrency || !render_pass) {
    return nullptr;
  }

  ShaderState *state = new ShaderState;
  state->gpu = gpu;
  state->glsl = glsl;

  // create
  {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkQueueFlags flags = rawkit_glsl_is_compute(glsl)
      ? VK_QUEUE_COMPUTE_BIT
      : VK_QUEUE_GRAPHICS_BIT;

    info.queueFamilyIndex = rawkit_vulkan_find_queue_family(gpu, flags);
    VkResult err = vkCreateCommandPool(
      gpu->device,
      &info,
      NULL,
      &state->command_pool
    );

    if (err) {
      printf("ERROR: ShaderState: could not create command pool (%i)\n", err);
      delete state;
      return nullptr;
    }
  }

  // create the shader modules
  {
    const uint8_t stage_count = rawkit_glsl_stage_count(glsl);
    for (uint8_t stage_idx=0; stage_idx<stage_count; stage_idx++) {
      VkShaderModuleCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      info.codeSize = rawkit_glsl_spirv_byte_len(glsl, stage_idx);
      info.pCode = rawkit_glsl_spirv_data(glsl, stage_idx);
      VkShaderModule module = VK_NULL_HANDLE;
      VkResult err = vkCreateShaderModule(
        gpu->device,
        &info,
        gpu->allocator,
        &module
      );

      if (err) {
        printf("ERROR: ShaderState: failed to create shader module (%i)\n", err);
        delete state;
        return nullptr;
      }

      state->modules.push_back(module);
    }
  }

  // create the descriptor pool
  {
    // TODO: compute these sizes from the shader, remember these are `descriptors` not descriptor sets
    //       which means the computed values need to be multiplied by the frame count
    vector<VkDescriptorPoolSize> pool_sizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * pool_sizes.size();
    pool_info.poolSizeCount = (uint32_t)pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    VkResult err = vkCreateDescriptorPool(
      gpu->device,
      &pool_info,
      gpu->allocator,
      &state->descriptor_pool
    );

    if (err) {
      printf("ERROR: rawkit-shader: could not create descriptor pool (%i)\n", err);
      delete state;
      return nullptr;
    }
  }

  // create the pipeline layout
  {
    VkResult err = state->create_pipeline_layout();
    if (err != VK_SUCCESS) {
      printf("ERROR: ShaderState: failed to create pipeline layout (%i)\n", err);
      delete state;
      return nullptr;
    }
  }

  // create the pipeline
  {
    VkResult err = VK_SUCCESS;
    if (rawkit_glsl_is_compute(glsl)){
      err = state->create_compute_pipeline();
    } else {
      err = state->create_graphics_pipeline(render_pass);
    }

    if (err != VK_SUCCESS) {
      printf("ERROR: ShaderState: failed to create pipeline (%i)\n", err);
      delete state;
      return nullptr;
    }
  }

  for (uint8_t i=0; i<concurrency; i++) {
    VkResult err = state->populate_concurrent_entries();
    if (err) {
      printf("ERROR: ShaderState: failed to populate concurrent entries #%u (%i)\n", i, err);
      delete state;
      return nullptr;
    }
  }

  return state;
}

ShaderState::~ShaderState() {
  this->concurrent_entries.clear();
  this->writes.clear();

  if (!this->gpu || !this->gpu->device) {
    return;
  }

  vkDeviceWaitIdle(this->gpu->device);

  if (this->descriptor_pool) {
    vkDestroyDescriptorPool(
      this->gpu->device,
      this->descriptor_pool,
      this->gpu->allocator
    );
  }

  if (this->command_pool) {
    vkDestroyCommandPool(
      this->gpu->device,
      this->command_pool,
      this->gpu->allocator
    );
  }

  for (VkDescriptorSetLayout layout : this->descriptor_set_layouts) {
    vkDestroyDescriptorSetLayout(this->gpu->device, layout, this->gpu->allocator);
  }
  this->descriptor_set_layouts.clear();

  if (this->pipeline_layout) {
    vkDestroyPipelineLayout(this->gpu->device, this->pipeline_layout, this->gpu->allocator);
  }

  for (VkShaderModule module : this->modules) {
    vkDestroyShaderModule(this->gpu->device, module, this->gpu->allocator);
  }

  if (this->pipeline) {
    vkDestroyPipeline(this->gpu->device, this->pipeline, this->gpu->allocator);
  }
}