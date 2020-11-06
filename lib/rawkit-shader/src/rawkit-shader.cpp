#include <stdlib.h>

#include <rawkit/gpu.h>
#include <rawkit/shader.h>
#include <rawkit/texture.h>
#include <rawkit/file.h>

#include <stb_sb.h>

#include <vector>
using namespace std;

int rawkit_shader_param_size(const rawkit_shader_param_t *param) {
  switch (param->type) {
    case RAWKIT_SHADER_PARAM_F32: return sizeof(float);
    case RAWKIT_SHADER_PARAM_I32: return sizeof(int32_t);
    case RAWKIT_SHADER_PARAM_U32: return sizeof(uint32_t);
    case RAWKIT_SHADER_PARAM_F64: return sizeof(double);
    case RAWKIT_SHADER_PARAM_I64: return sizeof(int64_t);
    case RAWKIT_SHADER_PARAM_U64: return sizeof(uint64_t);
    case RAWKIT_SHADER_PARAM_PTR: return param->bytes;
    default:
      return param->bytes;
  }
}

rawkit_shader_param_value_t rawkit_shader_param_value(rawkit_shader_param_t *param) {
  rawkit_shader_param_value_t ret = {};
  ret.len = rawkit_shader_param_size(param);
  ret.should_free = false;

  switch (param->type) {
    case RAWKIT_SHADER_PARAM_F32: {
      ret.buf = &param->f32;
      break;
    };
    case RAWKIT_SHADER_PARAM_I32: {
      ret.buf = &param->i32;
      break;
    }
    case RAWKIT_SHADER_PARAM_U32: {
      ret.buf = &param->u32;
      break;
    }
    case RAWKIT_SHADER_PARAM_F64: {
      ret.buf = &param->f64;
      break;
    }
    case RAWKIT_SHADER_PARAM_I64: {
      ret.buf = &param->i64;
      break;
    }
    case RAWKIT_SHADER_PARAM_U64: {
      ret.buf = &param->u64;
      break;
    }
    case RAWKIT_SHADER_PARAM_PTR: {
      ret.buf = param->ptr;
      break;
    }

    case RAWKIT_SHADER_PARAM_TEXTURE_PTR: {

      break;
    }

    case RAWKIT_SHADER_PARAM_PULL_STREAM: {
      if (param->pull_stream && param->pull_stream->fn) {
        // Note: we own this memory now
        ps_val_t *val = param->pull_stream->fn(param->pull_stream, PS_OK);

        if (val) {
          // TODO: this should probably be a destroy_fn(void *) like in pull-stream because the
          //       thing that created it likely knows how to destroy it properly.
          ret.should_free = true;
          ret.buf = val->data;
          ret.len = val->len;
        }
      }
      break;
    }

    case RAWKIT_SHADER_PARAM_UNIFORM_BUFFER: {
      ret.buf = param->ptr;
      break;
    }

    default:
      printf("ERROR: unhandled case in rawkit_shader_param_value\n");
  }

  return ret;
}

static VkDescriptorType rawkit_glsl_reflection_entry_to_vulkan_descriptor_type(const rawkit_glsl_reflection_entry_t *entry) {
  rawkit_glsl_reflection_entry_type type = entry->entry_type;

  switch (type) {
    case RAWKIT_GLSL_REFLECTION_ENTRY_SUBPASS_INPUT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    case RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_INPUT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    case RAWKIT_GLSL_REFLECTION_ENTRY_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case RAWKIT_GLSL_REFLECTION_ENTRY_SEPARATE_IMAGE: {
      if (entry->dims == RAWKIT_GLSL_DIMS_BUFFER) {
       return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
      }

      return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }

    case RAWKIT_GLSL_REFLECTION_ENTRY_SEPARATE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
    case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE: {
      if (entry->dims == RAWKIT_GLSL_DIMS_BUFFER) {
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
      }

      return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }

    // or VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
    //   A dynamic storage buffer (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) is almost identical to a storage buffer,
    //   and differs only in how the offset into the buffer is specified. The base offset calculated by the
    //   VkDescriptorBufferInfo when initially updating the descriptor set is added to a dynamic offset when binding
    //   the descriptor set.
    case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    // or VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
    //   A dynamic uniform buffer (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) is almost identical to a uniform buffer,
    //   and differs only in how the offset into the buffer is specified. The base offset calculated by the
    //   VkDescriptorBufferInfo when initially updating the descriptor set is added to a dynamic offset when binding
    //   the descriptor set.
    case RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case RAWKIT_GLSL_REFLECTION_ENTRY_ACCELERATION_STRUCTURE: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    default:
     return VK_DESCRIPTOR_TYPE_MAX_ENUM;
  }
}

// this is a redefinition of VkDescriptorSetLayoutCreateInfo so we can
// dynamically grow VkDescriptorSetLayoutBinding while processing reflection data.
typedef struct rawkit_descriptor_set_layout_create_info_t {
    VkStructureType                        sType;
    const void*                            pNext;
    VkDescriptorSetLayoutCreateFlags       flags;
    uint32_t                               bindingCount;
    VkDescriptorSetLayoutBinding*    pBindings;
} rawkit_descriptor_set_layout_create_info_t;

static VkShaderStageFlagBits stage_flags(rawkit_glsl_stage_mask_t stage) {
  switch (stage) {
    case RAWKIT_GLSL_STAGE_VERTEX_BIT: return VK_SHADER_STAGE_VERTEX_BIT;
    case RAWKIT_GLSL_STAGE_TESSELLATION_CONTROL_BIT: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case RAWKIT_GLSL_STAGE_TESSELLATION_EVALUATION_BIT: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case RAWKIT_GLSL_STAGE_GEOMETRY_BIT: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case RAWKIT_GLSL_STAGE_FRAGMENT_BIT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case RAWKIT_GLSL_STAGE_COMPUTE_BIT: return VK_SHADER_STAGE_COMPUTE_BIT;
    case RAWKIT_GLSL_STAGE_RAYGEN_BIT: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    case RAWKIT_GLSL_STAGE_ANY_HIT_BIT: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    case RAWKIT_GLSL_STAGE_CLOSEST_HIT_BIT: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    case RAWKIT_GLSL_STAGE_MISS_BIT: return VK_SHADER_STAGE_MISS_BIT_KHR;
    case RAWKIT_GLSL_STAGE_INTERSECTION_BIT: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    case RAWKIT_GLSL_STAGE_CALLABLE_BIT: return VK_SHADER_STAGE_CALLABLE_BIT_KHR;

    default: return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
  }
}

static VkShaderStageFlagBits stage_flags_at_index(rawkit_glsl_t *glsl, uint8_t stage_idx) {
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
}

static VkResult create_graphics_pipeline(rawkit_glsl_t *glsl, rawkit_shader_t *shader) {
  const uint8_t stage_count = rawkit_glsl_stage_count(glsl);
  vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos(stage_count);
  const char *entry_point = "main";
  for (uint8_t stage_idx=0; stage_idx<stage_count; stage_idx++) {
    pipelineShaderStageCreateInfos[stage_idx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfos[stage_idx].stage = stage_flags_at_index(glsl, stage_idx);
    pipelineShaderStageCreateInfos[stage_idx].module = shader->modules[stage_idx];
    pipelineShaderStageCreateInfos[stage_idx].pName = entry_point;
  }

  const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(glsl);
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

  VkRect2D scissor = {};
  scissor.extent.height = 400;
  scissor.extent.width = 400;
  scissor.offset.x = 0;
  scissor.offset.y = 0;

  VkViewport viewport = {};
  viewport.height = 400;
  viewport.width = 400;
  viewport.x = 0;
  viewport.y = 0;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkPipelineViewportStateCreateInfo         pViewportState = {};
  pViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pViewportState.scissorCount = 0;
  // pViewportState.pScissors = &scissor;
  pViewportState.viewportCount = 0;
  // pViewportState.pViewports = &viewport;

  VkPipelineRasterizationStateCreateInfo    pRasterizationState = {};
  pRasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  pRasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  pRasterizationState.lineWidth = 1.0f;
  pRasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
  pRasterizationState.cullMode = VK_CULL_MODE_NONE;

  VkPipelineMultisampleStateCreateInfo      pMultisampleState = {};
  pMultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  pMultisampleState.minSampleShading = 1.0f;

  VkPipelineDepthStencilStateCreateInfo     pDepthStencilState = {};
  pDepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

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

  vector<VkDynamicState> dynamicStates = {
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
  info.layout = shader->pipeline_layout;
  info.renderPass = rawkit_vulkan_renderpass();
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
    rawkit_vulkan_device(),
    rawkit_vulkan_pipeline_cache(),
    1,
    &info,
    NULL,
    &pipeline
  );
  if (err) {
    printf("ERROR: could not create graphics pipeline (%i)\n", err);
    return err;
  }

  shader->pipeline = pipeline;


  return VK_SUCCESS;
}

// TODO: output should come through as a param
VkResult rawkit_shader_init(rawkit_glsl_t *glsl, rawkit_shader_t *shader) {
  if (!shader || !glsl || !rawkit_glsl_valid(glsl)) {
    return VK_INCOMPLETE;
  }

  VkResult err = VK_SUCCESS;

  VkDevice device = rawkit_vulkan_device();

  VkPipelineCache pipeline_cache = rawkit_vulkan_pipeline_cache();

  // Create a command pool per shader pipeline.
  // TODO: there may be room here to share a pool, not sure what the best
  //       practice is.
  if (shader->command_pool == VK_NULL_HANDLE) {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = rawkit_vulkan_queue_family();
    err = vkCreateCommandPool(
      device,
      &info,
      NULL,
      &shader->command_pool
    );

    if (err) {
      printf("ERROR: could not create command pool\n");
      return err;
    }
  }

  // create the appropriate shader modules
  VkShaderModule *modules = NULL;
  {
    const uint8_t stage_count = rawkit_glsl_stage_count(glsl);
    for (uint8_t stage_idx=0; stage_idx<stage_count; stage_idx++) {
      VkShaderModuleCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      info.codeSize = rawkit_glsl_spirv_byte_len(glsl, stage_idx);
      info.pCode = rawkit_glsl_spirv_data(glsl, stage_idx);
      VkShaderModule module = VK_NULL_HANDLE;
      VkResult err = vkCreateShaderModule(
        device,
        &info,
        NULL, //NULL /* v->Allocator */,
        &module
      );

      if (err) {
        rawkit_glsl_destroy(glsl);
        printf("ERROR: failed to create shader module\n");
        return err;
      }

      sb_push(modules, module);
    }

    const uint32_t prev_module_count = sb_count(shader->modules);
    for (uint32_t i=0; i<prev_module_count; i++) {
      if (shader->modules[i] != VK_NULL_HANDLE) {
        vkDestroyShaderModule(
          rawkit_vulkan_device(),
          shader->modules[i],
          NULL
        );
      }
    }
    sb_free(shader->modules);
    shader->modules = modules;
  }

  const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(glsl);
  VkPushConstantRange pushConstantRange = {};
  // TODO: there can only be one push constant buffer per stage, so when this changes we'll need
  //       to use another mechanism to track
  pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
  pushConstantRange.offset = 0;
  pushConstantRange.size = 0;


  // aggregate the descriptor set layouts from reflection data
  {
    rawkit_descriptor_set_layout_create_info_t *dslci = NULL;

    for (uint32_t entry_idx=0; entry_idx<reflection.len; entry_idx++) {
      rawkit_glsl_reflection_entry_t *entry = &reflection.entries[entry_idx];

      // push constant ranges
      if (entry->entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER) {
        pushConstantRange.size += entry->block_size;
        continue;
      }

      if (entry->set < 0 || entry->binding < 0) {
        continue;
      }

      // setup ubos
      if (entry->entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER) {
        rawkit_shader_uniform_buffer_t ubo = {};
        ubo.entry = entry;
        ubo.set = entry->set;
        ubo.size = entry->block_size;

        // create the buffer
        {
          VkBufferCreateInfo create = {};
          create.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
          create.size = ubo.size;
          create.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
          create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
          err = vkCreateBuffer(device, &create, nullptr, &ubo.handle);
          if (err != VK_SUCCESS) {
            printf("ERROR: unable to create UBO buffer\n");
            return err;
          }
        }

        // create the memory to back the buffer
        {
          VkMemoryRequirements memRequirements;
          vkGetBufferMemoryRequirements(device, ubo.handle, &memRequirements);

          VkMemoryAllocateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
          info.allocationSize = memRequirements.size;
          int32_t memory_idx = rawkit_vulkan_find_memory_type(
            shader->physical_device,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            memRequirements.memoryTypeBits
          );

          if (memory_idx < 0) {
            printf("ERROR: could not locate the appropriate memory type for UBO\n");
            return VK_ERROR_UNKNOWN;
          }

          info.memoryTypeIndex = static_cast<uint32_t>(memory_idx);

          err = vkAllocateMemory(device, &info, nullptr, &ubo.memory);
          if (err != VK_SUCCESS) {
            printf("ERROR: could not allocate memory for UBO\n");
            return err;
          }
        }

        // bind them together
        vkBindBufferMemory(device, ubo.handle, ubo.memory, 0);

        sb_push(shader->ubos, ubo);
      }


      // allow descriptor set layouts to be sparse
      while (!dslci || entry->set >= sb_count(dslci)) {
        rawkit_descriptor_set_layout_create_info_t info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pBindings = NULL;
        info.bindingCount = 0;
        sb_push(dslci, info);
      }

      // add the binding for this entry
      {
        VkDescriptorSetLayoutBinding binding = {};
        binding.descriptorType = rawkit_glsl_reflection_entry_to_vulkan_descriptor_type(entry);
        binding.stageFlags = stage_flags(entry->stage);
        binding.binding = entry->binding;
        binding.descriptorCount = 1;
        sb_push(dslci[entry->set].pBindings, binding);
        dslci[entry->set].bindingCount = sb_count(dslci[entry->set].pBindings);
      }
    }

    shader->descriptor_set_layout_count = sb_count(dslci);

    if (shader->descriptor_set_layout_count) {
      if (shader->descriptor_set_layouts) {
        for (uint32_t i=0; i<shader->descriptor_set_layout_count; i++) {
          if (shader->descriptor_set_layouts[i] != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, shader->descriptor_set_layouts[i], NULL);
          }
        }

        free(shader->descriptor_set_layouts);
        shader->descriptor_set_layouts = NULL;
      }

      shader->descriptor_set_layouts = (VkDescriptorSetLayout *)calloc(
        sizeof(VkDescriptorSetLayout) * shader->descriptor_set_layout_count,
        1
      );

      if (!shader->descriptor_set_layouts) {
        printf("ERROR: could not allocate descriptor set layouts\n");
        return VK_ERROR_UNKNOWN;
      }

      for (uint32_t dslci_idx=0; dslci_idx<shader->descriptor_set_layout_count; dslci_idx++) {
        err = vkCreateDescriptorSetLayout(
          device,
          (VkDescriptorSetLayoutCreateInfo *)&dslci[dslci_idx],
          NULL,
          &shader->descriptor_set_layouts[dslci_idx]
        );

        if (err != VK_SUCCESS) {
          printf("ERROR: could not create descriptor layout [%u]\n", dslci_idx);
          return err;
        }

        sb_free(dslci[dslci_idx].pBindings);
      }

      sb_free(dslci);
    }
  }

  // create descriptor sets from layouts
  if (shader->descriptor_set_layout_count) {
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    // TODO: this is likely bad.
    descriptorSetAllocateInfo.descriptorPool = rawkit_vulkan_descriptor_pool();
    descriptorSetAllocateInfo.pSetLayouts = shader->descriptor_set_layouts;
    descriptorSetAllocateInfo.descriptorSetCount = shader->descriptor_set_layout_count;

    VkDescriptorSet *descriptor_sets = (VkDescriptorSet *)malloc(
      sizeof(VkDescriptorSet) * shader->descriptor_set_layout_count
    );

    if (!descriptor_sets) {
      printf("ERROR: unable to allocate memory for descriptor set pointers\n");
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    err = vkAllocateDescriptorSets(
      device,
      &descriptorSetAllocateInfo,
      descriptor_sets
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: unable to allocate descriptor sets\n");
      free(descriptor_sets);
      return err;
    }

    if (shader->descriptor_sets) {
      free(shader->descriptor_sets);
    }
    shader->descriptor_sets = descriptor_sets;
    shader->descriptor_set_count = shader->descriptor_set_layout_count;
  }

  {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = shader->descriptor_set_layout_count;
    pipelineLayoutCreateInfo.pSetLayouts = shader->descriptor_set_layouts;

    if (pushConstantRange.size > 0) {
      pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
      pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    }

    err = vkCreatePipelineLayout(
      device,
      &pipelineLayoutCreateInfo,
      NULL,
      &shader->pipeline_layout
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: unable to create pipeline layout\n");
      return err;
    }

    if (rawkit_glsl_is_compute(glsl)){
      if (!sb_count(shader->modules)) {
        printf("ERROR: shader->modules has len 0 when trying to build compute shader pipeline\n");;
        return VK_INCOMPLETE;
      }

      VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
      pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
      pipelineShaderStageCreateInfo.module = shader->modules[0];
      pipelineShaderStageCreateInfo.pName = "main";

      VkComputePipelineCreateInfo computePipelineCreateInfo = {};
      computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
      computePipelineCreateInfo.layout = shader->pipeline_layout;
      computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;

      err = vkCreateComputePipelines(
        device,
        pipeline_cache,
        1,
        &computePipelineCreateInfo,
        NULL,
        &shader->pipeline
      );


      if (err != VK_SUCCESS) {
        printf("ERROR: failed to create compute pipelines\n");
        return err;
      }
    } else {
      err = create_graphics_pipeline(
        glsl,
        shader
      );

      if (err != VK_SUCCESS) {
        printf("ERROR: failed to create graphics pipeline\n");
        return err;
      }
    }

    if (!shader->command_buffer) {
      // Create a command buffer for compute operations
      VkCommandBufferAllocateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      info.commandPool = shader->command_pool;
      info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      info.commandBufferCount = 1;

      err = vkAllocateCommandBuffers(
        device,
        &info,
        &shader->command_buffer
      );

      if (err != VK_SUCCESS) {
        printf("ERROR: failed to allocate command buffers\n");
        return err;
      }
    }
  }

  // bind uniform buffers to descriptor sets
  {
    uint32_t descriptor_set_count = sb_count(shader->descriptor_sets);
    uint32_t l = sb_count(shader->ubos);
    for (uint32_t i=0; i<l; i++) {
      VkDescriptorBufferInfo info = {};
      info.buffer = shader->ubos[i].handle;
      info.offset = 0;
      info.range = shader->ubos[i].size;

      uint32_t set_idx = shader->ubos[i].set;

      if (set_idx >= descriptor_set_count) {
        printf("ERROR: descriptor set out of range\n");
        return VK_ERROR_UNKNOWN;
      }

      VkWriteDescriptorSet write = {};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = shader->descriptor_sets[set_idx];
      write.dstBinding = 0;
      write.dstArrayElement = 0;
      write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write.descriptorCount = 1;
      write.pBufferInfo = &info;

      vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

      // this list should be stable now, so we can link it back to the entry
      shader->ubos[i].entry->user_index = i;
    }
  }

  shader->glsl = glsl;

  return VK_SUCCESS;
}

void rawkit_shader_update_ubo(rawkit_shader_t *shader, const char *name, uint32_t len, void *value) {

  rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(shader->glsl, name);
  if (entry.entry_type != RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER) {
    printf("ERROR: attempted to update ubo for item of different type (type: %i)\n", entry.entry_type);
    return;
  }

  uint32_t ubo_idx = entry.user_index;
  if (ubo_idx >= sb_count(shader->ubos)) {
    printf("ERROR: ubo_idx out of range\n");
    return;
  }

  rawkit_shader_uniform_buffer_t *ubo = &shader->ubos[ubo_idx];

  if (len > ubo->size) {
    printf("ERROR: failed to update ubo because incoming len is larger than the underlying buffer (in: %llu, buf: %llu)\n",
      len,
      ubo->size
    );
    return;
  }

  VkDevice device = rawkit_vulkan_device();
  void* dst;
  vkMapMemory(device, ubo->memory, 0, ubo->size, 0, &dst);
  memcpy(dst, value, len);
  vkUnmapMemory(device, ubo->memory);
}

void rawkit_shader_set_param(rawkit_shader_t *shader, const rawkit_shader_param_t *param) {
  // TODO: cache by name + hash of the param data (maybe?)
  VkDevice device = rawkit_vulkan_device();

  const rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(shader->glsl, param->name);
  switch (entry.entry_type) {
    case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE: {
      if (!param->texture || !param->texture->sampler || !param->texture->image_view) {
        return;
      }

      VkDescriptorImageInfo imageInfo = {};
      imageInfo.sampler = param->texture->sampler;
      imageInfo.imageView = param->texture->image_view;
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

      VkWriteDescriptorSet writeDescriptorSet = {};
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.dstSet = shader->descriptor_sets[entry.set];
      writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      writeDescriptorSet.dstBinding = entry.binding;
      writeDescriptorSet.pImageInfo = &imageInfo;
      writeDescriptorSet.descriptorCount = 1;
      vkUpdateDescriptorSets(
        device,
        1,
        &writeDescriptorSet,
        0,
        NULL
      );

      break;
    }
    default:
      printf("ERROR: unhandled entry type %i\n", entry.entry_type);
  }
}


static VkResult build_shaders(VkPhysicalDevice physical_device, uint8_t shader_count, rawkit_shader_t **shaders_ptr, rawkit_glsl_t *glsl) {
  if (!shaders_ptr) {
    return VK_INCOMPLETE;
  }

  if (*shaders_ptr == NULL) {
    *shaders_ptr = (rawkit_shader_t *)calloc(
      shader_count * sizeof(rawkit_shader_t),
      1
    );
  }

  rawkit_shader_t *shaders = *shaders_ptr;

  for (uint32_t idx=0; idx < shader_count; idx++) {
    shaders[idx].physical_device = physical_device;
    VkResult err = rawkit_shader_init(glsl, &shaders[idx]);

    if (err != VK_SUCCESS) {
      printf("ERROR: rawkit_shader_init failed (%i)\n", err);
      return err;
    }
  }

  return VK_SUCCESS;
}

typedef struct rawkit_shader_pipeline_state_t {
  rawkit_shader_t *shaders;
  rawkit_glsl_source_t sources[RAWKIT_GLSL_STAGE_COUNT];
  rawkit_glsl_t *glsl;
} rawkit_shader_pipeline_state_t;

rawkit_shader_t *_rawkit_shader_ex(
    const char *from_file,
    uv_loop_t *loop,
    rawkit_diskwatcher_t *watcher,

    VkPhysicalDevice physical_device,
    uint32_t shader_copies,
    uint8_t source_count,
    const char **source_files
) {
  VkResult err;

  char id[4096] = "rawkit::shader::pipeline ";
  for (uint8_t i=0; i<source_count; i++) {
    strcat(id, source_files[i]);
    strcat(id, " ");
  }

  rawkit_shader_pipeline_state_t *state = rawkit_hot_state(id, rawkit_shader_pipeline_state_t);

  // rebuild the pipeline if any of the shaders changed
  {
    rawkit_glsl_source_t sources[RAWKIT_GLSL_STAGE_COUNT];

    bool changed = false;
    bool ready = true;
    for (uint8_t i=0; i<source_count; i++) {
      const rawkit_file_t *file = _rawkit_file_ex(
        from_file,
        source_files[i],
        loop,
        watcher
      );

      if (!file) {
        if (!state->sources[i].data) {
          ready = false;
          continue;
        }

        sources[i] = state->sources[i];
        continue;
      }
      sources[i].filename = source_files[i];
      sources[i].data = (const char *)file->data;
      changed = true;
    }

    if (!ready) {
      // we're still waiting for the shader sources to load off of disk..
      return NULL;
    }

    if (changed) {
      rawkit_glsl_t *glsl = rawkit_glsl_compile(
        source_count,
        sources,
        NULL
      );

      if (!rawkit_glsl_valid(glsl)) {
        printf("ERROR: glsl failed to compile\n");
        return state->shaders;
      }

      memcpy(state->sources, sources, sizeof(sources));

      rawkit_glsl_destroy(state->glsl);
      state->glsl = glsl;

      err = build_shaders(
        physical_device,
        shader_copies,
        &state->shaders,
        glsl
      );

      if (err) {
        printf("ERROR: could not build shaders\n");
        return state->shaders;
      }
    }
  }

  return state->shaders;
}

void rawkit_shader_apply_params(rawkit_shader_t *shader, VkCommandBuffer command_buffer, rawkit_shader_params_t params) {
  for (uint32_t i = 0; i<params.count; i++) {
    rawkit_shader_param_t *param = &params.entries[i];
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
      shader->glsl,
      param->name
    );

    switch (entry.entry_type) {
      case RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER: {
        rawkit_shader_param_value_t val = rawkit_shader_param_value(param);
        rawkit_shader_update_ubo(
          shader,
          param->name,
          val.len,
          val.buf
        );

        break;
      }

      case RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER: {
        rawkit_shader_param_value_t val = rawkit_shader_param_value(param);
        vkCmdPushConstants(
          command_buffer,
          shader->pipeline_layout,
          VK_SHADER_STAGE_COMPUTE_BIT,
          entry.offset,
          val.len,
          val.buf
        );
        break;
      }

      case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE: {
        // TODO: this is a nasty hack to get hot reloading textures working. The issue is that
        //       we need to update the texture descriptor for all shaders .. including the one
        //       that is currently in flight.

        // update descriptor sets
        // rawkit_shader_param_t texture = {};
        // texture.name = param->name;
        // texture.type = RAWKIT_SHADER_PARAM_TEXTURE_PTR;
        // texture.ptr = param->texture,
        // textyre.bytes = sizeof(*param->texture) }
        rawkit_shader_set_param(
          shader,
          param
        );

        break;
      }

      default:
        printf("ERROR: unhandled entry type (%i) while setting shader params\n", entry.entry_type);
        break;
    }
  }
}