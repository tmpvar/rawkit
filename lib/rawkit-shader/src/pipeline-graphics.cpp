#include "shader-state.h"

#include "util.h"

static VkShaderStageFlagBits stage_flags_at_index(const rawkit_glsl_t *glsl, uint8_t stage_idx) {
  return stage_flags(rawkit_glsl_stage_at_index(glsl, stage_idx));
}

static VkFormat type_and_count_to_format(rawkit_glsl_base_type type, uint32_t count) {
  switch (type) {
    case RAWKIT_GLSL_BASE_TYPE_INT: {
      switch (count) {
        case 1: return VK_FORMAT_R32_SINT;
        case 2: return VK_FORMAT_R32G32_SINT;
        case 3: return VK_FORMAT_R32G32B32_SINT;
        case 4: return VK_FORMAT_R32G32B32A32_SINT;
      }
      break;
    }

    case RAWKIT_GLSL_BASE_TYPE_UINT: {
      switch (count) {
        case 1: return VK_FORMAT_R32_UINT;
        case 2: return VK_FORMAT_R32G32_UINT;
        case 3: return VK_FORMAT_R32G32B32_UINT;
        case 4: return VK_FORMAT_R32G32B32A32_UINT;
      }
      break;
    }

    case RAWKIT_GLSL_BASE_TYPE_INT64: {
      switch (count) {
        case 1: return VK_FORMAT_R64_SINT;
        case 2: return VK_FORMAT_R64G64_SINT;
        case 3: return VK_FORMAT_R64G64B64_SINT;
        case 4: return VK_FORMAT_R64G64B64A64_SINT;
      }
      break;
    }

    case RAWKIT_GLSL_BASE_TYPE_UINT64: {
      switch (count) {
        case 1: return VK_FORMAT_R64_UINT;
        case 2: return VK_FORMAT_R64G64_UINT;
        case 3: return VK_FORMAT_R64G64B64_UINT;
        case 4: return VK_FORMAT_R64G64B64A64_UINT;
      }
      break;
    }

    case RAWKIT_GLSL_BASE_TYPE_FLOAT: {
      switch (count) {
        case 1: return VK_FORMAT_R32_SFLOAT;
        case 2: return VK_FORMAT_R32G32_SFLOAT;
        case 3: return VK_FORMAT_R32G32B32_SFLOAT;
        case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
      }
      break;
    }

    case RAWKIT_GLSL_BASE_TYPE_DOUBLE: {
      switch (count) {
        case 1: return VK_FORMAT_R64_SFLOAT;
        case 2: return VK_FORMAT_R64G64_SFLOAT;
        case 3: return VK_FORMAT_R64G64B64_SFLOAT;
        case 4: return VK_FORMAT_R64G64B64A64_SFLOAT;
      }
      break;
    }

    default:
      printf("ERROR: unhandled base type %i", type);
      return VK_FORMAT_UNDEFINED;
  }

  return VK_FORMAT_UNDEFINED;
}

VkResult ShaderState::create_graphics_pipeline(VkRenderPass render_pass) {
  if (!this->valid()) {
    return VK_INCOMPLETE;
  }

  const uint8_t stage_count = rawkit_glsl_stage_count(this->glsl);
  vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos(stage_count);
  const char *entry_point = "main";
  for (uint8_t stage_idx=0; stage_idx<stage_count; stage_idx++) {
    pipelineShaderStageCreateInfos[stage_idx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfos[stage_idx].stage = stage_flags_at_index(this->glsl, stage_idx);
    pipelineShaderStageCreateInfos[stage_idx].module = this->modules[stage_idx];
    pipelineShaderStageCreateInfos[stage_idx].pName = entry_point;
  }

  const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(this->glsl);
  vector<VkVertexInputAttributeDescription> vertex_input_attributes;
  vector<VkVertexInputBindingDescription> vertex_input_bindings;

  {
    uint32_t binding = 0;
    for (uint32_t entry_idx=0; entry_idx<reflection.len; entry_idx++) {
      rawkit_glsl_reflection_entry_t *entry = &reflection.entries[entry_idx];
      if (entry->entry_type != RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_INPUT) {
        continue;
      }

      // bindings
      {
        VkVertexInputBindingDescription desc = {};
        desc.binding = binding++;
        desc.stride = entry->block_size;
        // TODO: this should be configurable, but how?
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertex_input_bindings.push_back(desc);
      }

      // attachments
      {
        VkFormat format = type_and_count_to_format(entry->base_type, entry->vecsize);
        for (uint32_t loc = 0; loc < entry->columns; loc++) {
          VkVertexInputAttributeDescription desc = {};
          desc.binding = entry->binding;
          desc.location = entry->location + loc;
          desc.format = format;
          desc.offset = entry->offset;
          vertex_input_attributes.push_back(desc);
        }
      }
    }
  }


  VkPipelineVertexInputStateCreateInfo      pVertexInputState = {};
  pVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  pVertexInputState.vertexBindingDescriptionCount = vertex_input_bindings.size();
  pVertexInputState.pVertexBindingDescriptions = vertex_input_bindings.data();
  pVertexInputState.vertexAttributeDescriptionCount = vertex_input_attributes.size();
  pVertexInputState.pVertexAttributeDescriptions = vertex_input_attributes.data();

  VkPipelineInputAssemblyStateCreateInfo    pInputAssemblyState = {};
  pInputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  pInputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineTessellationStateCreateInfo     pTessellationState = {};
  pTessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;


  VkPipelineViewportStateCreateInfo         pViewportState = {};
  pViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pViewportState.viewportCount = 1;
  pViewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo    pRasterizationState = {};
  pRasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  pRasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  pRasterizationState.lineWidth = 1.0f;
  pRasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  pRasterizationState.cullMode = VK_CULL_MODE_NONE;

  VkPipelineMultisampleStateCreateInfo      pMultisampleState = {};
  pMultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  pMultisampleState.minSampleShading = 1.0f;

  VkPipelineDepthStencilStateCreateInfo     pDepthStencilState = {};
  pDepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  pDepthStencilState.depthTestEnable = VK_TRUE;
  pDepthStencilState.depthWriteEnable = VK_TRUE;

  pDepthStencilState.minDepthBounds = 0.0f;
  pDepthStencilState.maxDepthBounds = 1.0f;
  pDepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
  pDepthStencilState.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo       pColorBlendState = {};
  pColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = (
    VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT
  );
  colorBlendAttachment.blendEnable = VK_FALSE;
  pColorBlendState.attachmentCount = 1;
  pColorBlendState.pAttachments = &colorBlendAttachment;

  array<VkDynamicState, 2> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo pDynamicState = {};
  pDynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pDynamicState.dynamicStateCount = dynamicStates.size();
  pDynamicState.pDynamicStates = dynamicStates.data();

  VkGraphicsPipelineCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  info.stageCount = pipelineShaderStageCreateInfos.size();
  info.pStages = pipelineShaderStageCreateInfos.data();
  info.subpass = 0;
  info.layout = this->pipeline_layout;
  info.renderPass = render_pass;
  info.pVertexInputState = &pVertexInputState;
  info.pInputAssemblyState = &pInputAssemblyState;
  info.pTessellationState = &pTessellationState;
  info.pViewportState = &pViewportState;
  info.pRasterizationState = &pRasterizationState;
  info.pMultisampleState = &pMultisampleState;
  info.pDepthStencilState = &pDepthStencilState;
  info.pColorBlendState = &pColorBlendState;
  info.pDynamicState = &pDynamicState;

  VkPipeline pipeline;
  VkResult err = vkCreateGraphicsPipelines(
    this->gpu->device,
    this->gpu->pipeline_cache,
    1,
    &info,
    this->gpu->allocator,
    &pipeline
  );
  if (err) {
    printf("ERROR: could not create graphics pipeline (%i)\n", err);
    return err;
  }

  this->pipeline = pipeline;
  return VK_SUCCESS;
}
