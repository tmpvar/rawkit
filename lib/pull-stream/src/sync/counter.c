#include <pull/stream.h>

#include <stdlib.h>
#include <string.h>

// counter
typedef struct counter_t {
  PS_FIELDS
  uint64_t value;
} counter_t;


static ps_val_t *counter_fn(ps_t *base, ps_stream_status status) {
  if (ps_status(base, status)) {
    return NULL;
  }

  counter_t *c = (counter_t *)base;

  ps_val_t *v = ps_create_value(ps_val_t, NULL);
  v->len = sizeof(c->value);

  v->data = calloc(v->len, 1);
  memcpy(v->data, (void *)&c->value, v->len);

  c->value++;

  return v;
}

ps_t *create_counter() {
  counter_t *c = ps_create_stream(counter_t, NULL);
  c->fn = counter_fn;
  return (ps_t *)c;
}
