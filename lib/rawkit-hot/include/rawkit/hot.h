#pragma once

#include <rawkit/hash.h>

#include <string.h>

#ifdef __cplusplus
  extern "C" {
#endif

  typedef struct rawkit_hot_context_t rawkit_hot_context_t;

  void *rawkit_hot_state_ctx(
    rawkit_hot_context_t *ctx,
    const char *name,
    uint64_t id,
    uint64_t len,
    void *data,
    uint8_t is_resource
  );

  void rawkit_hot_resource_destroy_ctx(rawkit_hot_context_t *ctx, uint64_t id);

  #define _rawkit_hot_state(name, id, len, data, is_resource) rawkit_hot_state_ctx(rawkit_default_hot_context(), name, id, len, data, is_resource)
  #define rawkit_hot_resource_destroy(id) rawkit_hot_resource_destroy_ctx(rawkit_default_hot_context(), id)

  // Allow overrides when in the context of a worker
  #if !defined(RAWKIT_WORKER)
    #define rawkit_default_hot_context() NULL
  #endif


  #define rawkit_hot_resource(name, t) ( \
    (t *)_rawkit_hot_state( \
      name, \
      rawkit_hash(strlen(name), (void *)name), \
      sizeof(t), \
      NULL, \
      1 \
    ) \
  )
  #define rawkit_hot_resource(name, t) ( \
    (t *)_rawkit_hot_state( \
      name, \
      rawkit_hash(strlen(name), (void *)name), \
      sizeof(t), \
      NULL, \
      1 \
    ) \
  )

  #define rawkit_hot_resource_id(name, id, t) ( \
    (t *)_rawkit_hot_state( \
      name, \
      id, \
      sizeof(t), \
      NULL, \
      1 \
    ) \
  )

  #define rawkit_hot_state(name, t) ( \
    (t *)_rawkit_hot_state( \
      name, \
      rawkit_hash(strlen(name), (void *)name), \
      sizeof(t), \
      NULL, \
      0 \
    ) \
  )

  // BUFFERS

  typedef struct rawkit_cpu_buffer_t {
    RAWKIT_RESOURCE_FIELDS

    void *data;
    uint64_t size;
  } rawkit_cpu_buffer_t;

  rawkit_cpu_buffer_t *rawkit_cpu_buffer(const char *name, uint64_t size);

#ifdef __cplusplus
  }
#endif
