#pragma once

#include <rawkit/jit.h>
#include <rawkit/shader.h>

void host_init_rawkit_shader(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_shader_ex", rawkit_shader_ex);

  rawkit_jit_add_export(jit, "rawkit_shader_param_value", rawkit_shader_param_value);
  rawkit_jit_add_export(jit, "rawkit_shader_param_size", rawkit_shader_param_size);

  rawkit_jit_add_export(jit, "rawkit_shader_glsl", rawkit_shader_glsl);

  // Shader Instances
  rawkit_jit_add_export(jit, "rawkit_shader_instance_begin", rawkit_shader_instance_begin);
  rawkit_jit_add_export(jit, "rawkit_shader_instance_param_texture", rawkit_shader_instance_param_texture);
  rawkit_jit_add_export(jit, "_rawkit_shader_instance_param_ubo", _rawkit_shader_instance_param_ubo);
  rawkit_jit_add_export(jit, "rawkit_shader_instance_apply_params", rawkit_shader_instance_apply_params);
  rawkit_jit_add_export(jit, "rawkit_shader_instance_end", rawkit_shader_instance_end);
}
