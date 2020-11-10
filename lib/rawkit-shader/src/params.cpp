#include "shader-state.h"

static void rawkit_shader_set_concurrent_param(
  ShaderState *shader_state,
  ConcurrentStateEntry *state,
  const rawkit_glsl_reflection_entry_t entry,
  VkCommandBuffer command_buffer,
  rawkit_shader_param_t *param,
  bool is_compute
) {
  rawkit_gpu_t *gpu = shader_state->gpu;

  // TODO: cache by name + hash of the param data (maybe?)
  switch (entry.entry_type) {
    case RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER: {
      rawkit_shader_param_value_t val = rawkit_shader_param_value(param);
      vkCmdPushConstants(
        command_buffer,
        shader_state->pipeline_layout,
        is_compute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS,
        entry.offset,
        val.len,
        val.buf
      );
      break;
    }

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

      break;
    }

    case RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER: {
      auto it = state->buffers.find(param->name);
      if (it == state->buffers.end()) {
        return;
      }

      VkResult err = rawkit_gpu_buffer_update(
        gpu,
        it->second,
        param->ptr,
        param->bytes
      );

      if (err) {
        return;
      }

      break;
    }

    default:
      printf("ERROR: unhandled entry type %i\n", entry.entry_type);
  }
}

void rawkit_shader_apply_params(
  rawkit_shader_t *shader,
  uint8_t concurrency_index,
  VkCommandBuffer command_buffer,
  rawkit_shader_params_t params
) {
  ShaderState *shader_state = (ShaderState *)shader->_state;
  if (!shader_state || shader_state->concurrent_entries.size() <= concurrency_index) {
    return;
  }

  ConcurrentStateEntry *state = shader_state->concurrent_entries[concurrency_index];
  bool is_compute = rawkit_glsl_is_compute(shader_state->glsl);

  for (uint32_t i = 0; i<params.count; i++) {
    rawkit_shader_param_t *param = &params.entries[i];
    const rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
      shader_state->glsl,
      param->name
    );

    rawkit_shader_set_concurrent_param(
      shader_state,
      state,
      entry,
      command_buffer,
      param,
      is_compute
    );
  }
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
      printf("ERROR: unhandled case in rawkit_shader_param_value (%i)\n", param->type);
  }

  return ret;
}