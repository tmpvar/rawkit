#include "shader-state.h"

VkResult ShaderState::create_compute_pipeline() {
  if (!this->valid()) {
    return VK_INCOMPLETE;
  }

  VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
  pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipelineShaderStageCreateInfo.module = this->modules[0];
  pipelineShaderStageCreateInfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo = {};
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = this->pipeline_layout;
  computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;

  VkResult err = vkCreateComputePipelines(
    this->gpu->device,
    this->gpu->pipeline_cache,
    1,
    &computePipelineCreateInfo,
    this->gpu->allocator,
    &this->pipeline
  );

  if (err != VK_SUCCESS) {
    printf("ERROR: failed to create compute pipelines\n");
    return err;
  }

  return VK_SUCCESS;
}