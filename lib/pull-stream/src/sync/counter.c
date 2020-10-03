#include <pull/stream.h>

#include <stdlib.h>
#include <stdint.h>

// counter
typedef struct counter_t {
  PS_CB_FIELDS
  uint64_t value;
} counter_t;

static ps_value_t *counter_cb(ps_cb_t *base, ps_status status) {
  counter_t *cb = (counter_t *)base;

  if (handle_status(base, status)) {
    return NULL;
  }

  ps_value_t *v = (ps_value_t *)calloc(sizeof(ps_value_t), 1);

  cb->value++;

  v->data = (void *)&cb->value;
  v->len = sizeof(cb->value);
  return v;
}

ps_cb_t *create_counter() {
  counter_t *c = (counter_t *)calloc(sizeof(counter_t), 1);
  c->fn = counter_cb;
  return (ps_cb_t *)c;
}
