#include <rawkit/glsl.h>
#include <rawkit/hot.h>
#include <rawkit/shader.h>
#include <rawkit/vulkan.h>

#include "shader-state.h"

static const char *resource_name = "rawkit::shader";
static rawkit_shader_t ERR = {};

rawkit_shader_t *rawkit_shader_ex(
  rawkit_gpu_t *gpu,
  uint8_t concurrency,
  VkRenderPass render_pass,
  uint8_t file_count,
  const rawkit_file_t **files
) {
  if (!file_count || !files) {
    printf("ERROR: rawkit_shader was provided invalid args\n");
    return &ERR;
  }

  const rawkit_glsl_t *glsl = rawkit_glsl_file_array(file_count, files);

  if (!file_count || !files) {
    return &ERR;
  }

  uint64_t id = rawkit_hash_resources(resource_name, file_count, (const rawkit_resource_t **)files);
  rawkit_shader_t *shader = rawkit_hot_resource_id(resource_name, id, rawkit_shader_t);
  if (!shader) {
    printf("ERROR: rawkit_shader experienced out of memory error while allocating\n");
    return &ERR;
  }

  bool dirty = rawkit_resource_sources_array((rawkit_resource_t *)shader, 1, (rawkit_resource_t **)&glsl);

  if (!dirty) {
    return shader;
  }

  ShaderState *state = ShaderState::create(gpu, glsl, concurrency, render_pass);
  if (!state) {
    printf("ERROR: rawkit-shader: failed to create ShaderState\n");
    return shader;
  }

  if (shader->_state) {
    ShaderState *old_state = (ShaderState *)shader->_state;
    delete old_state;
  }

  shader->_state = (void *)state;
  shader->resource_version++;
  return shader;
}

void rawkit_shader_bind(
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

  VkPipelineBindPoint bind_point = rawkit_glsl_is_compute(shader_state->glsl)
    ? VK_PIPELINE_BIND_POINT_COMPUTE
    : VK_PIPELINE_BIND_POINT_GRAPHICS;

  vkCmdBindPipeline(
    command_buffer,
    bind_point,
    shader_state->pipeline
  );

  rawkit_shader_apply_params(
    shader,
    concurrency_index,
    command_buffer,
    params
  );

  if (state->descriptor_sets.size() > 0) {
    vkCmdBindDescriptorSets(
      command_buffer,
      bind_point,
      shader_state->pipeline_layout,
      0,
      static_cast<uint32_t>(state->descriptor_sets.size()),
      state->descriptor_sets.data(),
      0,
      0
    );
  }
}

const rawkit_glsl_t *rawkit_shader_glsl(rawkit_shader_t *shader) {
  ShaderState *shader_state = (ShaderState *)shader->_state;
  if (!shader_state) {
    return NULL;
  }
  return shader_state->glsl;
}

VkPipelineStageFlags rawkit_glsl_vulkan_stage_flags(rawkit_glsl_stage_mask_t stage) {
  if (stage & RAWKIT_GLSL_STAGE_ALL) {
    return VK_SHADER_STAGE_ALL;
  }

  VkPipelineStageFlags ret = 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_VERTEX_BIT) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_TESSELLATION_CONTROL_BIT) ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_TESSELLATION_EVALUATION_BIT) ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_GEOMETRY_BIT) ? VK_SHADER_STAGE_GEOMETRY_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_FRAGMENT_BIT) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_COMPUTE_BIT) ? VK_SHADER_STAGE_COMPUTE_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_RAYGEN_BIT) ? VK_SHADER_STAGE_RAYGEN_BIT_KHR : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_ANY_HIT_BIT) ? VK_SHADER_STAGE_ANY_HIT_BIT_KHR : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_CLOSEST_HIT_BIT) ? VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_MISS_BIT) ? VK_SHADER_STAGE_MISS_BIT_KHR : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_INTERSECTION_BIT) ? VK_SHADER_STAGE_INTERSECTION_BIT_KHR : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_CALLABLE_BIT) ? VK_SHADER_STAGE_CALLABLE_BIT_KHR : 0;
  return ret;
}