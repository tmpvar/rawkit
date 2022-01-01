#pragma once

#include <rawkit/jit.h>
#include <rawkit/glsl.h>

static void host_init_rawkit_glsl(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_glsl_file_array", rawkit_glsl_file_array);
  rawkit_jit_add_export(jit, "_rawkit_glsl_va", _rawkit_glsl_va);
  rawkit_jit_add_export(jit, "rawkit_glsl_valid", rawkit_glsl_valid);
  rawkit_jit_add_export(jit, "rawkit_glsl_spirv_data", rawkit_glsl_spirv_data);
  rawkit_jit_add_export(jit, "rawkit_glsl_spirv_byte_len", rawkit_glsl_spirv_byte_len);
  rawkit_jit_add_export(jit, "rawkit_glsl_reflection_entries", rawkit_glsl_reflection_entries);
  rawkit_jit_add_export(jit, "rawkit_glsl_reflection_entry", rawkit_glsl_reflection_entry);
  rawkit_jit_add_export(jit, "rawkit_glsl_reflection_descriptor_set_max", rawkit_glsl_reflection_descriptor_set_max);
  rawkit_jit_add_export(jit, "rawkit_glsl_reflection_binding_count_for_set", rawkit_glsl_reflection_binding_count_for_set);
  rawkit_jit_add_export(jit, "rawkit_glsl_workgroup_size", rawkit_glsl_workgroup_size);
}