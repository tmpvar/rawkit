#pragma once

#include <rawkit/hash.h>

#include <string.h>

#ifdef __cplusplus
  extern "C" {
#endif

  void *_rawkit_hot_state(uint64_t id, uint64_t len, void *data, uint8_t is_resource);

  #define rawkit_hot_resource(name, t) ( \
    (t *)_rawkit_hot_state( \
      rawkit_hash(strlen(name), (void *)name), \
      sizeof(t), \
      NULL, \
      1 \
    ) \
  )

  #define rawkit_hot_state(name, t) ( \
    (t *)_rawkit_hot_state( \
      rawkit_hash(strlen(name), (void *)name), \
      sizeof(t), \
      NULL, \
      0 \
    ) \
  )

#ifdef __cplusplus
  }
#endif
