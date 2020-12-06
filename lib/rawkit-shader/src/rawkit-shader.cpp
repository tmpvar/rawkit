#include <rawkit/glsl.h>
#include <rawkit/hot.h>
#include <rawkit/shader.h>
#include <rawkit/vulkan.h>

#include "shader-state.h"

static rawkit_shader_t ERR = {};

rawkit_shader_t *rawkit_shader_ex(
  rawkit_gpu_t *gpu,
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

  string resource_name = "rawkit::shader(";
  for (uint8_t i=0; i<file_count; i++) {
    if (i>0) {
      resource_name+=", ";
    }
    resource_name += files[i]->resource_name;
  }
  resource_name += ")";


  uint64_t id = rawkit_hash_resources(resource_name.c_str(), file_count, (const rawkit_resource_t **)files);
  rawkit_shader_t *shader = rawkit_hot_resource_id(resource_name.c_str(), id, rawkit_shader_t);
  if (!shader) {
    printf("ERROR: rawkit_shader experienced out of memory error while allocating\n");
    return &ERR;
  }

  bool dirty = rawkit_resource_sources_array((rawkit_resource_t *)shader, 1, (rawkit_resource_t **)&glsl);

  ShaderState *current_state = (ShaderState *)shader->_state;
  uint32_t gpu_tick_idx = rawkit_gpu_get_tick_idx(gpu);
  if (current_state && current_state->gpu_tick_idx != gpu_tick_idx) {
    current_state->instance_idx = 0;
    current_state->gpu_tick_idx = gpu_tick_idx;
  }

  if (!dirty) {
    return shader;
  }

  ShaderState *state = ShaderState::create(gpu, glsl, render_pass);
  if (!state) {
    printf("ERROR: rawkit-shader: failed to create ShaderState\n");
    return shader;
  }

  if (current_state) {
    delete current_state;
  }

  shader->_state = (void *)state;
  shader->resource_version++;
  return shader;
}

const rawkit_glsl_t *rawkit_shader_glsl(rawkit_shader_t *shader) {
  ShaderState *shader_state = (ShaderState *)shader->_state;
  if (!shader_state) {
    return NULL;
  }
  return shader_state->glsl;
}

VkPipelineStageFlags rawkit_glsl_vulkan_stage_flags(rawkit_glsl_stage_mask_t stage) {
  VkPipelineStageFlags ret = 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_VERTEX_BIT) ? VK_PIPELINE_STAGE_VERTEX_SHADER_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_TESSELLATION_CONTROL_BIT) ? VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_TESSELLATION_EVALUATION_BIT) ? VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_GEOMETRY_BIT) ? VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_FRAGMENT_BIT) ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_COMPUTE_BIT) ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : 0;
  ret |= (stage & RAWKIT_GLSL_STAGE_ALL) ? VK_PIPELINE_STAGE_ALL_COMMANDS_BIT : 0;

  return ret;
}