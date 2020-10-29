#pragma once

#include <vulkan/vulkan.h>
#include "rawkit-texture.h"

#include <pull/stream.h>


typedef struct rawkit_shader_t {
  VkCommandBuffer command_buffer;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
  VkDescriptorSetLayout *descriptor_set_layouts;
  uint32_t descriptor_set_layout_count;
  VkDescriptorSet descriptor_set;
  VkWriteDescriptorSet write_descriptor_set;
  VkCommandPool command_pool;
  VkShaderModule shader_module;
} rawkit_shader_t;

static float push_constant_f32 = 1.0;
// RAWKIT_SHADER_ARG_COUNT - count the number of VA_ARGS in both c and c++ mode
#ifdef __cplusplus
  #include <tuple>
  #define RAWKIT_SHADER_ARG_COUNT(...) (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)
#else
  #define RAWKIT_SHADER_ARG_COUNT(...) ((int)(sizeof((rawkit_shader_param_t[]){ __VA_ARGS__ })/sizeof(rawkit_shader_param_t)))
#endif

#define rawkit_shader_push_constants(opts, ...) { \
  opts.count = RAWKIT_SHADER_ARG_COUNT(__VA_ARGS__); \
  opts.entries = (rawkit_shader_param_t[]){ \
    __VA_ARGS__ \
  }; \
}

typedef enum {
  RAWKIT_SHADER_PARAM_F32,
  RAWKIT_SHADER_PARAM_I32,
  RAWKIT_SHADER_PARAM_U32,

  RAWKIT_SHADER_PARAM_F64,
  RAWKIT_SHADER_PARAM_I64,
  RAWKIT_SHADER_PARAM_U64,

  RAWKIT_SHADER_PARAM_PTR,
  RAWKIT_SHADER_PARAM_TEXTURE_PTR,
  RAWKIT_SHADER_PARAM_PULL_STREAM,
} rawkit_shader_param_types_t;

#define rawkit_shader_f32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_F32, .f32 = _value, .bytes = 4, }
#define rawkit_shader_i32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_I32, .i32 = _value, .bytes = 4, }
#define rawkit_shader_u32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_U32, .u32 = _value, .bytes = 4, }

#define rawkit_shader_f64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_F64, .f64 = _value, .bytes = 8, }
#define rawkit_shader_i64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_I64, .i64 = _value, .bytes = 8, }
#define rawkit_shader_u64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_U64, .u64 = _value, .bytes = 8, }

#define rawkit_shader_array(_name, _len, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_ARRAY, .ptr = _value, .bytes = _len * sizeof(*_value), }
#define rawkit_shader_pull_stream(_name, _ps) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_PULL_STREAM, .ptr = _ps, .bytes = sizeof(_ps), }

typedef struct rawkit_shader_param_t {
  const char *name;
  uint32_t type;
  union {
    uint64_t value;
    float f32;
    float i32;
    float u32;
    double f64;
    double i64;
    double u64;

    rawkit_texture_t *texture;
    void *ptr;
    ps_t *pull_stream;

  };
  uint64_t bytes;
} rawkit_shader_param_t;

typedef struct rawkit_shader_param_value_t {
  bool should_free;
  void *buf;
  uint64_t len;
} rawkit_shader_param_value_t;

typedef struct rawkit_shader_params_t {
  uint32_t count;
  rawkit_shader_param_t *entries;
} rawkit_shader_params_t;

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
        // TODO: we own this memory now
        ps_val_t *val = param->pull_stream->fn(param->pull_stream, PS_OK);

        if (val) {
          ret.should_free = true;
          ret.buf = val->data;
          ret.len = val->len;
        }
      }
      break;
    }

    default:
      printf("ERROR: unhandled case in rawkit_shader_param_value\n");
  }

  return ret;
}

// TODO: output should come thorugh as a param
void rawkit_shader_init(
  rawkit_glsl_t *glsl,
  rawkit_shader_t *shader,
  const rawkit_shader_params_t *params,
  rawkit_texture_t *output
) {
  if (!shader) {
    return;
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

    if (err != VK_SUCCESS) {
      printf("ERROR: could not create command pool\n");
      return;
    }
  }

  if (shader->descriptor_set_layouts) {
    for (uint32_t i=0; i<shader->descriptor_set_layout_count; i++) {
      if (shader->descriptor_set_layouts[i] != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, shader->descriptor_set_layouts[i], NULL);
      }
    }

    free(shader->descriptor_set_layouts);
    shader->descriptor_set_layout_count = 0;
  }


  const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(glsl);

  uint32_t *descriptor_set_bindings_max = NULL;

  uint32_t descriptor_set_len = 0;
  if (reflection.len) {
    int32_t descriptor_set_max = -1;
    for (uint32_t i=0; i<reflection.len; i++) {
      const rawkit_glsl_reflection_entry_t *entry = &reflection.entries[i];

      if (entry->set > descriptor_set_max) {
        descriptor_set_max = entry->set;

      }
    }
    descriptor_set_len = (uint32_t)(max(descriptor_set_max + 1, 0));
  }


  // TODO: we need the max value set here and we'll alloc a sparse array
  //       and only fill the bits that we need.

  if (descriptor_set_len > 0) {
    // Descriptor set layout
    {
      shader->descriptor_set_layouts = (VkDescriptorSetLayout *)calloc(
        sizeof(VkDescriptorSetLayout) * descriptor_set_len,
        1
      );

      if (!shader->descriptor_set_layouts) {
        printf("Unable to allocate descriptor_set_layouts\n");
        return;
      }

      shader->descriptor_set_layout_count = descriptor_set_len;


      // TODO: handle sparse sets
      for (uint32_t i=0; i<descriptor_set_len; i++) {
        uint32_t bindings_per_set = rawkit_glsl_reflection_binding_count_for_set(glsl, i);
        VkDescriptorSetLayoutBinding *setLayoutBindings = ( VkDescriptorSetLayoutBinding *)calloc(
          bindings_per_set * sizeof(VkDescriptorSetLayoutBinding),
          1
        );

        printf("bindings per setllllll: %u\n", bindings_per_set);
        if (bindings_per_set) {
          // VkDescriptorSetLayoutBinding setLayoutBindings = {};
          setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
          setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
          setLayoutBindings[0].binding = 0;
          setLayoutBindings[0].descriptorCount = 1;

          // setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
          // setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
          // setLayoutBindings[1].binding = 1;
          // setLayoutBindings[1].descriptorCount = 1;
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings;
        descriptorSetLayoutCreateInfo.bindingCount = bindings_per_set;

        err = vkCreateDescriptorSetLayout(
          device,
          &descriptorSetLayoutCreateInfo,
          NULL,
          &shader->descriptor_set_layouts[i]
        );

        free(setLayoutBindings);
      }

      if (err != VK_SUCCESS) {
        printf("ERROR: could note create descriptor set layout\n");
        return;
      }
    }

    // Descriptor sets
    {
      VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
      descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      descriptorSetAllocateInfo.descriptorPool = rawkit_vulkan_descriptor_pool();
      descriptorSetAllocateInfo.pSetLayouts = shader->descriptor_set_layouts;
      descriptorSetAllocateInfo.descriptorSetCount = shader->descriptor_set_layout_count;

      err = vkAllocateDescriptorSets(
        device,
        &descriptorSetAllocateInfo,
        &shader->descriptor_set
      );

      if (err != VK_SUCCESS) {
        printf("ERROR: unable to allocate descriptor sets\n");
        return;
      }

      VkDescriptorImageInfo imageInfo = {};
      imageInfo.sampler = output->sampler;
      imageInfo.imageView = output->image_view;
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

      VkWriteDescriptorSet writeDescriptorSet = {};
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.dstSet = shader->descriptor_set;
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
    }
  }

  VkPushConstantRange *pushConstantRanges = NULL;
  if (params->count) {
    uint32_t push_constant_offset = 0;
    pushConstantRanges = (VkPushConstantRange *)calloc(sizeof(VkPushConstantRange), 1);
    for (uint32_t i = 0; i<params->count; i++) {
      // TODO: this should come from SPIRV-cross reflection data
      pushConstantRanges[i].offset = push_constant_offset;
      pushConstantRanges[i].size = rawkit_shader_param_size(&params->entries[i]);
      push_constant_offset += pushConstantRanges[i].size;
    }
  }

  {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = shader->descriptor_set_layout_count;
    pipelineLayoutCreateInfo.pSetLayouts = shader->descriptor_set_layouts;

    pipelineLayoutCreateInfo.pushConstantRangeCount = params->count;
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges;


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

    free(pushConstantRanges);

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
    pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
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
}

