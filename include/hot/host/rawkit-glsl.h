#pragma once

#include <rawkit/jit.h>
#include <rawkit/glsl.h>

void host_init_rawkit_glsl(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_glsl_paths_destroy", (void *)&rawkit_glsl_paths_destroy);
  rawkit_jit_add_export(jit, "rawkit_glsl_compile", (void *)&rawkit_glsl_compile);
  rawkit_jit_add_export(jit, "rawkit_glsl_destroy", (void *)&rawkit_glsl_destroy);
  rawkit_jit_add_export(jit, "rawkit_glsl_compiler", (void *)&rawkit_glsl_compiler);
  rawkit_jit_add_export(jit, "rawkit_glsl_valid", (void *)&rawkit_glsl_valid);
  rawkit_jit_add_export(jit, "rawkit_glsl_spirv_data", (void *)&rawkit_glsl_spirv_data);
  rawkit_jit_add_export(jit, "rawkit_glsl_spirv_byte_len", (void *)&rawkit_glsl_spirv_byte_len);
  rawkit_jit_add_export(jit, "rawkit_glsl_reflection_entries", (void *)&rawkit_glsl_reflection_entries);
  rawkit_jit_add_export(jit, "rawkit_glsl_reflection_descriptor_set_count", (void *)&rawkit_glsl_reflection_descriptor_set_count);
  rawkit_jit_add_export(jit, "rawkit_glsl_reflection_binding_count_for_set", (void *)&rawkit_glsl_reflection_binding_count_for_set);
}