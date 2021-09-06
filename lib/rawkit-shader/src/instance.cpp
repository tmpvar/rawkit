#pragma optimize("", off)
#include <rawkit/shader.h>
#include "shader-state.h"

#include <string>
#include <vector>
using namespace std;


static inline VkResult update_descriptor_set_buffer(
  rawkit_gpu_t *gpu,
  ShaderInstanceState *state,
  rawkit_glsl_reflection_entry_t *reflection_entry,
  rawkit_gpu_buffer_t *buffer
) {

  if (!buffer || !buffer->handle || !buffer->memory) {
    printf("ERROR: update_descriptor_set_buffer: buffer was invalid\n");
    return VK_ERROR_UNKNOWN;
  }

  VkDescriptorBufferInfo info = {};
  info.buffer = buffer->handle;
  info.offset = 0;
  info.range = reflection_entry->block_size;

  uint32_t set_idx = reflection_entry->set;

  if (set_idx >= state->descriptor_sets.size()) {
    printf("ERROR: descriptor set out of range\n");
    return VK_ERROR_UNKNOWN;
  }

  VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
  switch (reflection_entry->entry_type) {
    case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER:
      descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    case RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER:
      descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    default:
      printf("ERROR: attempting to update buffer for reflection entry that is not a buffer\n");
      return VK_ERROR_UNKNOWN;
  }

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = state->descriptor_sets[set_idx];
  write.dstBinding = reflection_entry->binding;
  write.dstArrayElement = 0;
  write.descriptorType = descriptor_type;
  write.descriptorCount = 1;
  write.pBufferInfo = &info;

  vkUpdateDescriptorSets(gpu->device, 1, &write, 0, nullptr);
  return VK_SUCCESS;
}


ShaderInstanceState::ShaderInstanceState(rawkit_shader_instance_t *instance) {
  this->instance = instance;
  this->shader = instance->shader;

  ShaderState *shader_state = (ShaderState *)shader->_state;

  const rawkit_glsl_t *glsl = rawkit_shader_glsl(shader);
  rawkit_gpu_t *gpu = instance->gpu;

  for (VkDescriptorSetLayout layout : shader_state->descriptor_set_layouts) {
    VkDescriptorSetAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = shader_state->descriptor_pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet set;
    VkResult err = vkAllocateDescriptorSets(
      gpu->device,
      &info,
      &set
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: unable to allocate descriptor sets (%i)\n", err);
      continue;
    }

    this->descriptor_sets.push_back(set);
  }

  // create buffers
  {
    const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(glsl);
    for (uint32_t entry_idx=0; entry_idx<reflection.len; entry_idx++) {
      rawkit_glsl_reflection_entry_t *reflection_entry = &reflection.entries[entry_idx];

      switch (reflection_entry->entry_type) {
        case RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER: {
          string buffer_name(shader->resource_name);
          buffer_name += "/";
          buffer_name += (reflection_entry->name);
          buffer_name += "/uniform-buffer";
          rawkit_gpu_buffer_t *buffer = rawkit_gpu_buffer_create(
            buffer_name.c_str(),
            gpu,
            reflection_entry->block_size,
            (
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
              VK_BUFFER_USAGE_TRANSFER_DST_BIT
            ),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
          );

          this->buffers.emplace(reflection_entry->name, buffer);

          VkResult err = update_descriptor_set_buffer(gpu, this, reflection_entry, buffer);
          if (err) {
            continue;
          }
          break;
        }
      }
    }
  }
}

ShaderInstanceState::~ShaderInstanceState() {
  this->descriptor_sets.clear();
  // NOTE: this does not manage the buffer lifecycle
  this->buffers.clear();
}

rawkit_shader_instance_t *rawkit_shader_instance_create_ex(
  rawkit_gpu_t *gpu,
  rawkit_shader_t *shader,
  rawkit_gpu_queue_t queue,
  uint32_t frame_idx
) {
  if (!gpu || !shader || !shader->_state || !shader->resource_version) {
    return NULL;
  }

  VkResult err = VK_SUCCESS;

  ShaderState *shader_state = (ShaderState *)shader->_state;
  uint64_t instance_idx = shader_state->instance_idx++;

  char instance_name[255] = "\0";
  sprintf(instance_name, "?instance=%llu&frame=%u", instance_idx, frame_idx);

  vector<uint64_t> keys = {
    shader->resource_id,
    rawkit_hash(strlen(instance_name), (void *)instance_name),
    static_cast<uint64_t>(frame_idx)
  };

  uint64_t resource_id = rawkit_hash_composite(keys.size(), keys.data());


  string id = string(shader->resource_name) + instance_name;

  rawkit_shader_instance_t *instance = rawkit_hot_resource_id(
    id.c_str(),
    resource_id,
    rawkit_shader_instance_t
  );

  instance->can_launch = true;
  instance->gpu = gpu;
  instance->shader = shader;
  instance->queue = queue;
  return instance;
}

rawkit_shader_instance_t *rawkit_shader_instance_begin_ex(
  rawkit_shader_instance_t *instance,
  VkCommandBuffer command_buffer
) {
  if (!instance || !instance->shader) {
    return nullptr;
  }
  auto gpu = instance->gpu;
  ShaderState *shader_state = (ShaderState *)instance->shader->_state;

  if (!command_buffer) {
    instance->owns_command_buffer = true;
    instance->command_buffer = rawkit_gpu_create_command_buffer(
      gpu,
      instance->queue.command_pool
    );
  } else {
    instance->owns_command_buffer = false;
    instance->command_buffer = command_buffer;
  }

  if (!instance->command_buffer) {
    printf("ERROR: rawkit_shader_instance_begin: could not create command buffer\n");
    return instance;
  }

  if (instance->owns_command_buffer) {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult err = vkBeginCommandBuffer(
      instance->command_buffer,
      &info
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: could not begin command buffer");
      return NULL;
    }
  }

  VkPipelineBindPoint bind_point = rawkit_glsl_is_compute(shader_state->glsl)
    ? VK_PIPELINE_BIND_POINT_COMPUTE
    : VK_PIPELINE_BIND_POINT_GRAPHICS;

  vkCmdBindPipeline(
    instance->command_buffer,
    bind_point,
    shader_state->pipeline
  );

  bool dirty = rawkit_resource_sources_array(
    (rawkit_resource_t *)instance,
    1,
    (rawkit_resource_t **)&instance->shader
  );


  ShaderInstanceState *instance_state = (ShaderInstanceState *)instance->_state;
  if (dirty) {
    if (instance_state) {
      delete instance_state;
    }

    instance_state = new ShaderInstanceState(instance);
    instance->_state = (void *)instance_state;
    instance->resource_version++;
  }

  if (instance_state->descriptor_sets.size() > 0) {
    vkCmdBindDescriptorSets(
      instance->command_buffer,
      bind_point,
      shader_state->pipeline_layout,
      0,
      static_cast<uint32_t>(instance_state->descriptor_sets.size()),
      instance_state->descriptor_sets.data(),
      0,
      0
    );
  }
  return instance;
}

void rawkit_shader_instance_param_texture(
  rawkit_shader_instance_t *instance,
  const char *name,
  rawkit_texture_t *texture,
  const rawkit_texture_sampler_t *sampler
) {
  if (!instance || !instance->_state || !texture || !name) {
    return;
  }

  if (!texture->resource_version) {
    instance->can_launch = false;
    return;
  }

  rawkit_gpu_t *gpu = instance->gpu;
  const rawkit_glsl_t *glsl = rawkit_shader_glsl(instance->shader);

  const rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
    glsl,
    name
  );

  VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

  switch (entry.entry_type) {
    case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE:
      descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      break;
    case RAWKIT_GLSL_REFLECTION_ENTRY_SAMPLED_IMAGE:
      //descriptor_type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
      descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;
    default:
      printf("WARN: rawkit_shader_instance_param_texture: could not update '%s', texture type not handled\n", name);
      return;
  }

  ShaderInstanceState *state = (ShaderInstanceState *)instance->_state;
  ShaderState *shader_state = (ShaderState *)state->shader->_state;

  // Note: we only transition for tetures not being bound to a compute shader.
  //       transitioning images while in a renderpass requires more coordination
  //       and so we've pushed that responsibility to the caller / a higher level
  //       abstraction.
  if (!shader_state->render_pass) {
    VkImageMemoryBarrier barrier = {};
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (entry.readable) {
      barrier.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT;
    }

    if (entry.writable) {
      barrier.dstAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
    }

    if (entry.writable || descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
      barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    } else {
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // TODO: this throws validation errors when not in general.
    // barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

    if (texture->options.is_depth) {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      // Stencil aspect should only be set on depth + stencil formats
      // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
      // if (texture->options.format >= VK_FORMAT_D16_UNORM_S8_UINT) {
      //   barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      // }
    } else {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    rawkit_texture_transition(
      texture,
      instance->command_buffer,
      rawkit_glsl_vulkan_stage_flags(entry.stage),
      barrier
    );
  }
  // update destriptor set
  {
    VkDescriptorImageInfo imageInfo = {};
    if (sampler) {
      imageInfo.sampler = sampler->handle;
    } else {
      imageInfo.sampler = texture->default_sampler->handle;
    }

    imageInfo.imageView = texture->image_view;
    imageInfo.imageLayout = texture->image_layout;

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = state->descriptor_sets[entry.set];
    writeDescriptorSet.descriptorType = descriptor_type;
    writeDescriptorSet.dstBinding = entry.binding;
    writeDescriptorSet.pImageInfo = &imageInfo;
    writeDescriptorSet.descriptorCount = 1;
    vkUpdateDescriptorSets(
      gpu->device,
      1,
      &writeDescriptorSet,
      0,
      NULL
    );

  }
}

void _rawkit_shader_instance_param_ubo(
  rawkit_shader_instance_t *instance,
  const char *name,
  void *data,
  uint64_t bytes
) {
  if (!instance || !instance->_state || !data || !bytes || !name) {
    return;
  }

  rawkit_gpu_t *gpu = instance->gpu;
  const rawkit_glsl_t *glsl = rawkit_shader_glsl(instance->shader);

  const rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
    glsl,
    name
  );
  if (entry.entry_type != RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER) {
    printf("WARN: rawkit_shader_instance_param_ubo: could not set '%s' as UBO\n", name);
    return;
  }

  ShaderInstanceState *state = (ShaderInstanceState *)instance->_state;
  auto it = state->buffers.find(name);
  if (it == state->buffers.end()) {
    return;
  }

  VkResult err = rawkit_gpu_buffer_update(
    it->second,
    data,
    bytes
  );
}

void rawkit_shader_instance_param_buffer_ex(
  rawkit_shader_instance_t *instance,
  const char *name,
  rawkit_gpu_buffer_t *buffer,
  VkDeviceSize offset,
  VkDeviceSize size
) {
  if (!instance || !instance->_state || !buffer || !buffer->handle || !name) {
    printf("WARN: rawkit_shader_instance_param_buffer: invalid instance for args name(%s) buffer(%s)\n", name, buffer ? buffer->resource_name : "<invalid>");
    printf("      shader(%s)\n", instance ? instance->resource_name : "<invalid>");
    return;
  }

  rawkit_gpu_t *gpu = instance->gpu;
  const rawkit_glsl_t *glsl = rawkit_shader_glsl(instance->shader);

  const rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
    glsl,
    name
  );
  if (entry.entry_type != RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER) {
    printf("WARN: rawkit_shader_instance_param_buffer: could not set '%s' as Storage Buffer (entry_type: %i)\n", name, entry.entry_type);
    return;
  }

  // update destriptor set
  {
    if (!buffer || !buffer->handle) {
      printf("ERROR: rawkit_shader_instance_param_buffer: buffer was invalid\n");
      return;
    }

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer->handle;
    bufferInfo.offset = offset;
    bufferInfo.range = size;

    ShaderInstanceState *state = (ShaderInstanceState *)instance->_state;

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = state->descriptor_sets[entry.set];
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.dstBinding = entry.binding;
    writeDescriptorSet.pBufferInfo = &bufferInfo;
    writeDescriptorSet.descriptorCount = 1;
    vkUpdateDescriptorSets(
      gpu->device,
      1,
      &writeDescriptorSet,
      0,
      NULL
    );
  }
}

void rawkit_shader_instance_param_ssbo_ex(
  rawkit_shader_instance_t *instance,
  const char *name,
  rawkit_gpu_ssbo_t *ssbo,
  VkDeviceSize offset,
  VkDeviceSize size
) {
  if (!instance || !instance->_state || !ssbo || !ssbo->resource_version || !name) {
    return;
  }

  if (!ssbo->buffer || !ssbo->staging_buffer) {
    return;
  }

  rawkit_shader_instance_param_buffer_ex(
    instance,
    name,
    ssbo->buffer,
    offset,
    size
  );
}

VkFence rawkit_shader_instance_end_ex(rawkit_shader_instance_t *instance) {
  if (!instance || !instance->can_launch || !instance->queue.handle) {
    return VK_NULL_HANDLE;
  }

  VkResult err = VK_SUCCESS;
  rawkit_gpu_t *gpu = instance->gpu;

  // Submit compute commands
  if (instance->owns_command_buffer) {
    err = vkEndCommandBuffer(instance->command_buffer);
    if (err != VK_SUCCESS) {
      printf("ERROR: vkEndCommandBuffer: failed %i\n", err);
      return VK_NULL_HANDLE;
    }

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkSubmitInfo computeSubmitInfo = {};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &instance->command_buffer;
    computeSubmitInfo.pWaitDstStageMask = &waitStageMask;

    VkFence fence;
    {
      err = rawkit_gpu_fence_create(gpu, &fence);
      if (err) {
        printf("ERROR: fill_rect: create fence failed (%i)\n", err);
        return VK_NULL_HANDLE;
      }
    }

    err = vkQueueSubmit(
      instance->queue.handle,
      1,
      &computeSubmitInfo,
      fence
    );

    rawkit_gpu_queue_command_buffer_for_deletion(
      gpu,
      instance->command_buffer,
      fence,
      instance->queue.command_pool
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: unable to submit compute shader (%s) (%i)\n", instance->resource_name, err);
      return VK_NULL_HANDLE;
    }
    return fence;
  }
  return VK_NULL_HANDLE;
}


void rawkit_shader_instance_param_push_constants(
  rawkit_shader_instance_t *instance,
  const void *data,
  uint64_t bytes
) {
  if (!instance || !instance->shader) {
    return;
  }

  rawkit_shader_t *shader = instance->shader;
  ShaderState *shader_state = (ShaderState *)shader->_state;

  bool is_compute = rawkit_glsl_is_compute(shader_state->glsl);

  vkCmdPushConstants(
    instance->command_buffer,
    shader_state->pipeline_layout,
    // TODO: dodge a validation error:
    // vkCmdPushConstants(): stageFlags (VK_SHADER_STAGE_COMPUTE_BIT, offset (0), and size (16),
    // must contain all stages in overlapping VkPushConstantRange stageFlags
    is_compute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS,
    // VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_ALL_GRAPHICS,
    0,
    bytes,
    data
  );
}


void rawkit_shader_instance_apply_params(
  rawkit_shader_instance_t *instance,
  rawkit_shader_params_t params
) {
  if (!instance || !instance->shader) {
    return;
  }

  rawkit_shader_t *shader = instance->shader;
  ShaderState *shader_state = (ShaderState *)shader->_state;

  for (uint32_t i = 0; i<params.count; i++) {
    rawkit_shader_param_t *param = &params.entries[i];
    const rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
      shader_state->glsl,
      param->name
    );
    bool is_compute = rawkit_glsl_is_compute(shader_state->glsl);

    switch (entry.entry_type) {
      case RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER:
          _rawkit_shader_instance_param_ubo(instance, param->name, param->ptr, param->bytes);
        break;

      case RAWKIT_GLSL_REFLECTION_ENTRY_SAMPLED_IMAGE:
      case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE:
          rawkit_shader_instance_param_texture(instance, param->name, param->texture.texture, param->texture.sampler);
        break;

      case RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER: {
        rawkit_shader_param_value_t val = rawkit_shader_param_value(param);
        vkCmdPushConstants(
          instance->command_buffer,
          shader_state->pipeline_layout,
          is_compute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS,
          entry.offset,
          val.len,
          val.buf
        );
        break;
      }

      default:
        printf("ERROR: rawkit_shader_instance_apply_params: entry type (%i) not implemented (%s)\n", entry.entry_type, param->name);
    }
  }
}


void rawkit_shader_instance_dispatch_compute(
  rawkit_shader_instance_t *instance,
  uint32_t width,
  uint32_t height,
  uint32_t depth
) {
  if (!instance || !instance->shader || !instance->command_buffer) {
    return;
  }

  rawkit_shader_t *shader = instance->shader;
  const rawkit_glsl_t *glsl = rawkit_shader_glsl(shader);
  bool is_compute = rawkit_glsl_is_compute(glsl);

  if (!is_compute) {
    printf("ERROR: rawkit_shader_instance_dispatch_compute: '%s' is not a compute shader!\n", shader->resource_name);
    return;
  }

  const uint32_t *workgroup_size = rawkit_glsl_workgroup_size(glsl, 0);
  float local[3] = {
    (float)workgroup_size[0],
    (float)workgroup_size[1],
    (float)workgroup_size[2],
  };

  float global[3] = {
    (float)width,
    (float)height,
    (float)depth,
  };

  vkCmdDispatch(
    instance->command_buffer,
    (uint32_t)fmaxf(ceilf(global[0] / local[0]), 1.0),
    (uint32_t)fmaxf(ceilf(global[1] / local[1]), 1.0),
    (uint32_t)fmaxf(ceilf(global[2] / local[2]), 1.0)
  );
}
