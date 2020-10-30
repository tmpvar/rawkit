#pragma once

#include <rawkit/jit.h>
#include <rawkit/texture.h>

void host_init_rawkit_texture(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_texture_destroy", rawkit_texture_destroy);
  rawkit_jit_add_export(jit, "rawkit_texture_init", rawkit_texture_init);
  rawkit_jit_add_export(jit, "rawkit_texture_hot", rawkit_texture_hot);
}
