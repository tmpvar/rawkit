#include <pull/stream.h>

#include <stdlib.h>

// counter
typedef struct counter_t {
  PS_FIELDS
  uint64_t value;
} counter_t;

static ps_val_t *counter_fn(ps_t *base, ps_stream_status status) {
  if (ps_status(base, status)) {
    return NULL;
  }

  counter_t *cb = (counter_t *)base;
  cb->value++;

  ps_val_t *v = (ps_val_t *)calloc(sizeof(ps_val_t), 1);
  v->data = (void *)&cb->value;
  v->len = sizeof(cb->value);
  return v;
}

ps_t *create_counter() {
  counter_t *c = (counter_t *)calloc(sizeof(counter_t), 1);
  c->fn = counter_fn;
  return (ps_t *)c;
}
