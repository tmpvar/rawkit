#pragma once

#include <rawkit/jit.h>
#include <rawkit/shader.h>

void host_init_rawkit_shader(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_shader_param_value", rawkit_shader_param_value);
  rawkit_jit_add_export(jit, "rawkit_shader_init", rawkit_shader_init);
  rawkit_jit_add_export(jit, "rawkit_shader_apply_params", rawkit_shader_apply_params);
  rawkit_jit_add_export(jit, "rawkit_shader_set_param", rawkit_shader_set_param);
  rawkit_jit_add_export(jit, "rawkit_shader_update_ubo", rawkit_shader_update_ubo);
  rawkit_jit_add_export(jit, "_rawkit_shader_ex", _rawkit_shader_ex);
}
