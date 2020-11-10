#pragma once

#include <stdint.h>

#include <pull/stream.h>
#include <rawkit/core.h>

#ifdef __cplusplus
  extern "C" {
#endif

// TODO: provide _ex version for more control
uint64_t rawkit_hash(uint64_t len, void *data);
uint64_t rawkit_hash_resources(const char *name, uint32_t resource_count, const rawkit_resource_t **resources);

// Pull Stream based hashing
typedef struct ps_rawkit_hasher_t ps_rawkit_hasher_t;

ps_t *ps_rawkit_hasher();
uint64_t ps_rawkit_get_hash64(ps_t *s);


#ifdef __cplusplus
  }
#endif
