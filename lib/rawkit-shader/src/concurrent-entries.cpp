#include "shader-state.h"

ConcurrentStateEntry::ConcurrentStateEntry() {

}

ConcurrentStateEntry::~ConcurrentStateEntry() {
  this->descriptor_sets.clear();

  for (auto &it : this->buffers) {
    rawkit_gpu_buffer_destroy(this->gpu, it.second);
  }
  this->buffers.clear();
}


static inline VkResult update_descriptor_set_buffer(
  rawkit_gpu_t *gpu,
  ConcurrentStateEntry *state_entry,
  rawkit_glsl_reflection_entry_t *reflection_entry,
  rawkit_gpu_buffer_t *buffer
) {
  VkDescriptorBufferInfo info = {};
  info.buffer = buffer->handle;
  info.offset = 0;
  info.range = reflection_entry->block_size;

  uint32_t set_idx = reflection_entry->set;

  if (set_idx >= state_entry->descriptor_sets.size()) {
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
  write.dstSet = state_entry->descriptor_sets[set_idx];
  write.dstBinding = reflection_entry->binding;
  write.dstArrayElement = 0;
  write.descriptorType = descriptor_type;
  write.descriptorCount = 1;
  write.pBufferInfo = &info;

  vkUpdateDescriptorSets(gpu->device, 1, &write, 0, nullptr);
  return VK_SUCCESS;
}

VkResult ShaderState::populate_concurrent_entries() {
  ConcurrentStateEntry *entry = new ConcurrentStateEntry();
  entry->gpu = this->gpu;

  // Create the command buffer
  {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = this->command_pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    VkResult err = vkAllocateCommandBuffers(
      this->gpu->device,
      &info,
      &entry->command_buffer
    );

    if (err) {
      printf("ERROR: ShaderState: could not create command buffer (%i)\n", err);
      return err;
    }
  }

  for (VkDescriptorSetLayout layout : this->descriptor_set_layouts) {
    VkDescriptorSetAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = this->descriptor_pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet set;
    VkResult err = vkAllocateDescriptorSets(
      this->gpu->device,
      &info,
      &set
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: unable to allocate descriptor sets\n");
      return err;
    }

    entry->descriptor_sets.push_back(set);
  }

  // create buffers
  {
    const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(this->glsl);
    for (uint32_t entry_idx=0; entry_idx<reflection.len; entry_idx++) {
      rawkit_glsl_reflection_entry_t *reflection_entry = &reflection.entries[entry_idx];

      switch (reflection_entry->entry_type) {
        case RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER: {
          rawkit_gpu_buffer_t *buffer = rawkit_gpu_buffer_create(
            this->gpu,
            reflection_entry->block_size,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
          );

          entry->buffers.emplace(reflection_entry->name, buffer);

          VkResult err = update_descriptor_set_buffer(this->gpu, entry, reflection_entry, buffer);
          if (err) {
            return err;
          }
          break;
        }
      }
    }
  }

  this->concurrent_entries.push_back(entry);

  return VK_SUCCESS;
}