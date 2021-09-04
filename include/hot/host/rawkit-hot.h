#pragma once

#include <rawkit/jit.h>
#include <rawkit/hot.h>


static void host_init_rawkit_hot(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_hot_state_ctx", (void *)&rawkit_hot_state_ctx);
  rawkit_jit_add_export(jit, "rawkit_hot_resource_destroy_ctx", (void *)&rawkit_hot_resource_destroy_ctx);
  rawkit_jit_add_export(jit, "rawkit_cpu_buffer", (void *)&rawkit_cpu_buffer);
}
