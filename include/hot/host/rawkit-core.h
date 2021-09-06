#pragma once

#include <rawkit/jit.h>
#include <rawkit/core.h>

static void host_init_rawkit_core(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "_rawkit_resource_sources", (void *)&_rawkit_resource_sources);
  rawkit_jit_add_export(jit, "rawkit_resource_sources_array", (void *)&rawkit_resource_sources_array);
  rawkit_jit_add_export(jit, "rawkit_now", (void *)rawkit_now);
}