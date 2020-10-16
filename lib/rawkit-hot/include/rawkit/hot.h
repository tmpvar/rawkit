#pragma once

#include <rawkit/hash.h>

#include <string.h>

#ifdef __cplusplus
  extern "C" {
#endif

  void *_rawkit_hot_state(uint64_t id, uint64_t len, void *data);
  #define rawkit_hot_state(name, t) ( \
    (t *)_rawkit_hot_state( \
      rawkit_hash(strlen(name), (void *)name), \
      sizeof(t), \
      NULL \
    ) \
  )

#ifdef __cplusplus
  }
#endif
