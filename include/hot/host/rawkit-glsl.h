#pragma once

#include <rawkit/jit.h>
#include <rawkit/glsl.h>

void host_init_rawkit_glsl(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_glsl_paths_destroy", (void *)&rawkit_glsl_paths_destroy);
  rawkit_jit_add_export(jit, "rawkit_glsl_compile", (void *)&rawkit_glsl_compile);
  rawkit_jit_add_export(jit, "rawkit_glsl_destroy", (void *)&rawkit_glsl_destroy);
  rawkit_jit_add_export(jit, "rawkit_glsl_compiler", (void *)&rawkit_glsl_compiler);
}