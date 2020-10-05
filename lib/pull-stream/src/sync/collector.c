#include <pull/stream.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct collector_t {
  PS_FIELDS

  ps_val_t *buffer;
} collector_t;

static ps_val_t *collector_fn(ps_t *base, ps_stream_status status) {
  if (base->status) {
    return NULL;
  }

  collector_t *collector = (collector_t *)base;
  if (!collector->buffer) {
    ps_status(base, PS_DONE);
    return NULL;
  }

  ps_val_t *val = ps_pull(base, status);


  switch (base->status) {
    // drop the buffer and return null
    case PS_ERR: {
      if (!collector->buffer->data) {
        return NULL;
      }

      ps_destroy(collector->buffer);
      collector->buffer = NULL;
      return NULL;
    }

    // copy the contents of val into collector->buffer
    case PS_OK: {

      // it is ok that val sometimes comes back as null, it likely means
      // that the source stream is async and we are waiting for data
      if (!val) {
        return NULL;
      }

      uint64_t len = collector->buffer->len;
      uint64_t new_len = len + val->len;

      void *data = realloc(collector->buffer->data, new_len);
      if (data == NULL) {
        ps_status(base, PS_ERR);
        ps_destroy(collector->buffer);
        collector->buffer = NULL;
        ps_destroy(val);
        return NULL;
      }

      memcpy((uint8_t *)data + len, val->data, val->len);

      collector->buffer->data = data;
      collector->buffer->len = new_len;

      ps_destroy(val);
      return NULL;
    }

    // output the buffer
    case PS_DONE: {
      ps_val_t *buffer = collector->buffer;
      collector->buffer = NULL;
      collector->status = PS_OK;
      return buffer;
    }
  }

  return NULL;
}

void collector_destroy_fn(ps_handle_t *base) {
  if (!base) {
    return;
  }

  collector_t *collector = (collector_t *)base;
  ps_destroy(collector->buffer);
  free(base);
}

ps_t *create_collector() {
  collector_t *collector = ps_create_stream(collector_t, collector_destroy_fn);

  collector->fn = collector_fn;
  collector->buffer = (ps_val_t *)calloc(sizeof(ps_val_t), 1);
  collector->buffer->handle_destroy_fn = free;

  return (ps_t *)collector;
}
