#pragma once

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