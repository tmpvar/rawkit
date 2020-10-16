#pragma once


#include <rawkit/jit.h>
#include <rawkit/hash.h>

void host_init_rawkit_hash(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_hash", (void *)&rawkit_hash);
  rawkit_jit_add_export(jit, "ps_rawkit_hasher", (void *)&ps_rawkit_hasher);
  rawkit_jit_add_export(jit, "ps_rawkit_get_hash64", (void *)&ps_rawkit_get_hash64);
}