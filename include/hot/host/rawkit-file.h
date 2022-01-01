#pragma once

#include <rawkit/jit.h>
#include <rawkit/file.h>

static void host_init_rawkit_file(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "_rawkit_file_ex", _rawkit_file_ex);
}