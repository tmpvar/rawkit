#pragma once

#include <rawkit/jit.h>
#include <rawkit/image.h>

void host_init_rawkit_texture(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "_rawkit_image_ex", _rawkit_image_ex);
}
