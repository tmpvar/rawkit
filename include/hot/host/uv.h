#pragma once

#include <rawkit/jit.h>

#include <uv.h>

void host_init_uv(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "uv_default_loop", (void *)&uv_default_loop);
}