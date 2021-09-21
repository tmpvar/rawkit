#include "shader-state.h"
#include <rawkit-glsl-internal.h>
#include "util.h"

typedef vector<VkDescriptorSetLayoutBinding> SetBindingVector;

VkResult ShaderState::create_pipeline_layout() {
  if (!this->valid()) {
    return VK_INCOMPLETE;
  }


  vector<VkPushConstantRange> push_constant_ranges;
  vector<SetBindingVector> sets;
  // create descriptor set layout bindings
  {
    const rawkit_glsl_reflection_vector_t reflection = rawkit_glsl_reflection_entries(glsl);

    for (uint32_t entry_idx=0; entry_idx<reflection.len; entry_idx++) {
      rawkit_glsl_reflection_entry_t *entry = &reflection.entries[entry_idx];

      // push constant ranges
      if (entry->entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER) {
        const auto c = push_constant_ranges.size();
        if (c == 0) {
          VkPushConstantRange range = {};
          range.offset = u32(entry->offset);
          range.size = entry->block_size;
          range.stageFlags = stage_flags(entry->stage);
          push_constant_ranges.push_back(range);
          continue;
        }

        auto &last = push_constant_ranges[c-1];
        if (last.offset + last.size == entry->offset) {
          last.size += entry->block_size;
          continue;
        }

        VkPushConstantRange range = {};
        range.offset = u32(entry->offset);
        range.size = entry->block_size;
        range.stageFlags = stage_flags(entry->stage);
        push_constant_ranges.push_back(range);

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

    if (push_constant_ranges.size()) {
      info.pushConstantRangeCount = push_constant_ranges.size();
      info.pPushConstantRanges = push_constant_ranges.data();
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

