#pragma once


#include <rawkit/jit.h>
#include <rawkit/hash.h>

static void host_init_rawkit_hash(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_hash", rawkit_hash);
  rawkit_jit_add_export(jit, "ps_rawkit_hasher", ps_rawkit_hasher);
  rawkit_jit_add_export(jit, "ps_rawkit_get_hash64", ps_rawkit_get_hash64);
  rawkit_jit_add_export(jit, "rawkit_hash_composite", rawkit_hash_composite);
}