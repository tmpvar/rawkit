#pragma once

#include <rawkit/jit.h>
#include <rawkit/texture.h>

void host_init_rawkit_texture(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_texture_destroy", rawkit_texture_destroy);
  rawkit_jit_add_export(jit, "rawkit_texture_init", rawkit_texture_init);
  rawkit_jit_add_export(jit, "_rawkit_texture_ex", _rawkit_texture_ex);
  rawkit_jit_add_export(jit, "rawkit_texture_sampler", rawkit_texture_sampler);
  rawkit_jit_add_export(jit, "rawkit_texture_target_begin", rawkit_texture_target_begin);
  rawkit_jit_add_export(jit, "rawkit_texture_target_end", rawkit_texture_target_end);
}
