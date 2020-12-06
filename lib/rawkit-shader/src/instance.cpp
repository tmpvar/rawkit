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


ShaderInstanceState::ShaderInstanceState(rawkit_shader_t *shader, rawkit_shader_instance_t *instance) {
  this->instance = instance;

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
          rawkit_gpu_buffer_t *buffer = rawkit_gpu_buffer_create(
            gpu,
            reflection_entry->block_size,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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

  for (auto &it : this->buffers) {
    rawkit_gpu_buffer_destroy(this->instance->gpu, it.second);
  }
  this->buffers.clear();
}


rawkit_shader_instance_t *rawkit_shader_instance_begin(rawkit_gpu_t *gpu, rawkit_shader_t *shader, VkCommandBuffer command_buffer, uint32_t frame_idx) {
  if (!gpu || !shader || !shader->_state) {
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

  if (!command_buffer) {
    instance->owns_command_buffer = true;
    instance->command_pool = gpu->command_pool;
    instance->command_buffer = rawkit_gpu_create_command_buffer(gpu);
  } else {
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
    err = vkBeginCommandBuffer(
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
    (rawkit_resource_t **)&shader
  );


  ShaderInstanceState *instance_state = (ShaderInstanceState *)instance->_state;
  if (dirty) {
    if (instance_state) {
      delete instance_state;
    }

    instance_state = new ShaderInstanceState(shader, instance);
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

static VkResult transition_texture_for_stage(
  rawkit_gpu_t *gpu,
  rawkit_texture_t *texture,
  VkImageMemoryBarrier barrier,
  VkPipelineStageFlags stageFlags
) {
  VkResult err = VK_SUCCESS;
  VkCommandBuffer command_buffer = rawkit_gpu_create_command_buffer(gpu);
  if (!command_buffer) {
    printf("ERROR: transition_texture_for_compute: could not create command buffer\n");
    return VK_INCOMPLETE;
  }
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VkResult begin_result = vkBeginCommandBuffer(command_buffer, &begin_info);

  rawkit_texture_transition(
    texture,
    command_buffer,
    stageFlags,
    barrier
  );

  {
    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;
    err = vkEndCommandBuffer(command_buffer);
    if (err) {
      printf("ERROR: transition_texture_for_stage: could not end command buffer");
      return err;
    }

    VkFence fence;
    {
      VkFenceCreateInfo create = {};
      create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      create.flags = 0;
      err = vkCreateFence(gpu->device, &create, gpu->allocator, &fence);
      if (err) {
        printf("ERROR: transition_texture_for_stage: create fence failed (%i)\n", err);
        return err;
      }
    }

    err = vkQueueSubmit(gpu->graphics_queue, 1, &end_info, fence);
    rawkit_gpu_queue_command_buffer_for_deletion(gpu, command_buffer, fence, gpu->command_pool);
    if (err) {
      printf("ERROR: transition_texture_for_stage: could not submit command buffer");
      return err;
    }
  }

  return err;
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
  if (entry.entry_type != RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE) {
    printf("WARN: rawkit_shader_instance_param_ubo: could not set '%s' as storage image\n", name);
    return;
  }

  ShaderInstanceState *state = (ShaderInstanceState *)instance->_state;

  VkImageMemoryBarrier barrier = {};
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  if (entry.writable) {
    barrier.dstAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  } else {
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  // TODO: this throws validation errors when not in general.
  barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

  transition_texture_for_stage(
    gpu,
    texture,
    barrier,
    rawkit_glsl_vulkan_stage_flags(entry.stage)
  );

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
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
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
    gpu,
    it->second,
    data,
    bytes
  );
}

void rawkit_shader_instance_end(rawkit_shader_instance_t *instance, VkQueue queue) {
  if (!instance || !instance->can_launch || !queue) {
    return;
  }

  VkResult err = VK_SUCCESS;
  rawkit_gpu_t *gpu = instance->gpu;

  // Submit compute commands
  if (instance->owns_command_buffer) {
    err = vkEndCommandBuffer(instance->command_buffer);
    if (err != VK_SUCCESS) {
      printf("ERROR: vkEndCommandBuffer: failed %i\n", err);
      return;
    }

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkSubmitInfo computeSubmitInfo = {};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &instance->command_buffer;
    computeSubmitInfo.pWaitDstStageMask = &waitStageMask;

    VkFence fence;
    {
      VkFenceCreateInfo create = {};
      create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      create.flags = 0;
      err = vkCreateFence(gpu->device, &create, gpu->allocator, &fence);
      if (err) {
        printf("ERROR: fill_rect: create fence failed (%i)\n", err);
        return;
      }
    }

    err = vkQueueSubmit(
      queue,
      1,
      &computeSubmitInfo,
      fence
    );

    rawkit_gpu_queue_command_buffer_for_deletion(
      gpu,
      instance->command_buffer,
      fence,
      instance->command_pool
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: unable to submit compute shader\n");
      return;
    }
  }
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

