#pragma once

#include <rawkit/jit.h>
#include <rawkit/hot.h>


void host_init_rawkit_hot(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "_rawkit_hot_state", (void *)&_rawkit_hot_state);
  rawkit_jit_add_export(jit, "rawkit_cpu_buffer", (void *)&rawkit_cpu_buffer);
}
