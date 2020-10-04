#include <pull/stream.h>

#include <stdlib.h>
#include <stdint.h>

typedef struct collector_t {
  PS_FIELDS

  ps_val_t *buffer;
} collector_t;

static ps_val_t *collector_fn(ps_t *base, ps_status status) {
  if (base->status) {
    return NULL;
  }

  collector_t *collector = (collector_t *)base;
  if (!collector->buffer) {
    handle_status(base, PS_DONE);
    return NULL;
  }

  ps_val_t *val = pull_through(base, status);

  switch (base->status) {
    // drop the buffer and return null
    case PS_ERR: {
      if (!collector->buffer->data) {
        return NULL;
      }

      ps_val_destroy(collector->buffer);
      collector->buffer = NULL;
      return NULL;
    }

    // copy the contents of val into collector->buffer
    case PS_OK: {
      uint64_t len = collector->buffer->len;
      uint64_t new_len = len + val->len;

      void *data = realloc(collector->buffer->data, new_len);
      if (data == NULL) {
        handle_status(base, PS_ERR);
        ps_val_destroy(collector->buffer);
        collector->buffer = NULL;
        ps_val_destroy(val);
        return NULL;
      }

      memcpy((uint8_t *)data + len, val->data, val->len);

      collector->buffer->data = data;
      collector->buffer->len = new_len;

      ps_val_destroy(val);
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

void collector_destroy_fn(ps_t *base) {
  if (!base) {
    return;
  }

  collector_t *collector = (collector_t *)base;
  ps_val_destroy(collector->buffer);
  free(base);
}

ps_t *create_collector() {
  collector_t *collector = (collector_t *)calloc(sizeof(collector_t), 1);

  collector->fn = collector_fn;
  collector->destroy_fn = collector_destroy_fn;
  collector->buffer = (ps_val_t *)calloc(sizeof(ps_val_t), 1);
  collector->buffer->destroy_fn = free;
  return (ps_t *)collector;
}
