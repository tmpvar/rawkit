#include <stdlib.h>

#include <rawkit/gpu.h>
#include <rawkit/shader.h>
#include <rawkit/texture.h>

#include <stb_sb.h>

inline int32_t memory_type_index(VkPhysicalDevice dev, uint32_t filter, VkMemoryPropertyFlags flags) {
  VkPhysicalDeviceMemoryProperties props;
  vkGetPhysicalDeviceMemoryProperties(dev, &props);

  for (int32_t i = 0; i < props.memoryTypeCount; i++) {
    if ((filter & (1 << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags) {
      return i;
    }
  }

  return -1;
}

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


// TODO: output should come thorugh as a param
void rawkit_shader_init(rawkit_glsl_t *glsl, rawkit_shader_t *shader, const rawkit_shader_params_t *params) {
  if (!shader || !shader->shader_module || !glsl) {
    return;
  }

  shader->glsl = glsl;

  VkResult err = VK_SUCCESS;

  VkDevice device = rawkit_vulkan_device();

  VkPipelineCache pipeline_cache = rawkit_vulkan_pipeline_cache();

  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  VkShaderStageFlagBits pipelineStage = VK_SHADER_STAGE_COMPUTE_BIT;

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

    if (err != VK_SUCCESS) {
      printf("ERROR: could not create command pool\n");
      return;
    }
  }

  const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(shader->glsl);
  VkPushConstantRange pushConstantRange = {};
  // TODO: there can only be one push constant buffer per stage, so when this changes we'll need
  //       to use another mechanism to track
  pushConstantRange.stageFlags = stageFlags;
  pushConstantRange.offset = 0;
  pushConstantRange.size = 0;


  // compute the descriptor set layouts from reflection data
  {
    rawkit_descriptor_set_layout_create_info_t *dslci = NULL;

    for (uint32_t entry_idx=0; entry_idx<reflection.len; entry_idx++) {
      rawkit_glsl_reflection_entry_t *entry = &reflection.entries[entry_idx];

      // compute push constant ranges
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

          if (vkCreateBuffer(device, &create, nullptr, &ubo.handle) != VK_SUCCESS) {
            printf("ERROR: unable to create UBO buffer\n");
            return;
          }
        }

        // create the memory to back the buffer
        {
          VkMemoryRequirements memRequirements;
          vkGetBufferMemoryRequirements(device, ubo.handle, &memRequirements);

          VkMemoryAllocateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
          info.allocationSize = memRequirements.size;
          int32_t memory_idx = memory_type_index(
            shader->physical_device,
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
          );

          if (memory_idx < 0) {
            printf("ERROR: could not locate the appropriate memory type for UBO\n");
            return;
          }

          info.memoryTypeIndex = static_cast<uint32_t>(memory_idx);

          err = vkAllocateMemory(device, &info, nullptr, &ubo.memory);
          if (err != VK_SUCCESS) {
            printf("ERROR: could not allocate memory for UBO\n");
            return;
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
        // TODO: wire this up
        binding.stageFlags = stageFlags;
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
        return;
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
          return;
        }

        sb_free(dslci[dslci_idx].pBindings);
      }

      sb_free(dslci);
    }
  }

  // create descriptor sets from layouts
  {
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
      return;
    }

    err = vkAllocateDescriptorSets(
      device,
      &descriptorSetAllocateInfo,
      descriptor_sets
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: unable to allocate descriptor sets\n");
      free(descriptor_sets);
      return;
    }

    if (shader->descriptor_sets) {
      free(shader->descriptor_sets);
    }
    shader->descriptor_sets = descriptor_sets;
    shader->descriptor_set_count = shader->descriptor_set_layout_count;

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
      return;
    }

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
    pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo.stage = pipelineStage;
    pipelineShaderStageCreateInfo.module = shader->shader_module;
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
      return;
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
        return;
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
        return;
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

void rawkit_shader_set_param(rawkit_shader_t *shader, rawkit_shader_param_t param) {
  // TODO: cache by name + hash of the param data (maybe?)
  VkDevice device = rawkit_vulkan_device();

  const rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(shader->glsl, param.name);
  switch (entry.entry_type) {
    case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE: {
      if (false || !param.texture || !param.texture->sampler || !param.texture->image_view) {
        return;
      }

      VkDescriptorImageInfo imageInfo = {};
      imageInfo.sampler = param.texture->sampler;
      imageInfo.imageView = param.texture->image_view;
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

      VkWriteDescriptorSet writeDescriptorSet = {};
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.dstSet = shader->descriptor_sets[entry.set];
      writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      writeDescriptorSet.dstBinding = 0;
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
