#pragma once

#include <rawkit/jit.h>
#include <rawkit/shader.h>

void host_init_rawkit_shader(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_shader_ex", rawkit_shader_ex);
  rawkit_jit_add_export(jit, "rawkit_shader_param_value", rawkit_shader_param_value);
  rawkit_jit_add_export(jit, "rawkit_shader_param_size", rawkit_shader_param_size);
  rawkit_jit_add_export(jit, "rawkit_shader_apply_params", rawkit_shader_apply_params);
  rawkit_jit_add_export(jit, "rawkit_shader_bind", rawkit_shader_bind);
  rawkit_jit_add_export(jit, "rawkit_shader_glsl", rawkit_shader_glsl);
  rawkit_jit_add_export(jit, "rawkit_shader_params_init_resource", rawkit_shader_params_init_resource);
}
