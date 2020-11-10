#include "shader-state.h"
#include "util.h"

typedef vector<VkDescriptorSetLayoutBinding> SetBindingVector;

VkResult ShaderState::create_pipeline_layout() {
  if (!this->valid()) {
    return VK_INCOMPLETE;
  }

  VkPushConstantRange pushConstantRange = {};
  // TODO: there can only be one push constant buffer per stage, so when this changes we'll need
  //       to use another mechanism to track
  pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
  pushConstantRange.offset = 0;
  pushConstantRange.size = 0;

  vector<SetBindingVector> sets;
  // create descriptor set layout bindings
  {
    const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(glsl);

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

      while (sets.size() <= entry->set) {
        SetBindingVector set;
        sets.push_back(set);
      }

      VkDescriptorSetLayoutBinding o = {};
      o.descriptorType = rawkit_glsl_reflection_entry_to_vulkan_descriptor_type(entry);
      o.stageFlags = stage_flags(entry->stage);
      o.binding = entry->binding;
      o.descriptorCount = 1;
      sets[entry->set].push_back(o);
    }
  }

  // create descriptor set layouts
  {
    for (auto &set : sets) {
      VkDescriptorSetLayout layout;

      VkDescriptorSetLayoutCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      if (set.size() > 0) {
        info.bindingCount = static_cast<uint32_t>(set.size());
        info.pBindings = set.data();
      }

      VkResult err = vkCreateDescriptorSetLayout(
        this->gpu->device,
        &info,
        this->gpu->allocator,
        &layout
      );

      if (err != VK_SUCCESS) {
        printf("ERROR: could not create descriptor layout\n");
        return err;
      }

      this->descriptor_set_layouts.push_back(layout);
    }
  }

  // create the pipeline layout
  {
    VkPipelineLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = static_cast<uint32_t>(this->descriptor_set_layouts.size());
    info.pSetLayouts = this->descriptor_set_layouts.data();

    if (pushConstantRange.size > 0) {
      info.pushConstantRangeCount = 1;
      info.pPushConstantRanges = &pushConstantRange;
    }

    VkResult err = vkCreatePipelineLayout(
      this->gpu->device,
      &info,
      this->gpu->allocator,
      &this->pipeline_layout
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: ShaderState: unable to create pipeline layout\n");
      return err;
    }
  }

  return VK_SUCCESS;
}

