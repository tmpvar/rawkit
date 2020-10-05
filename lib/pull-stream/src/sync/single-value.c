#include <pull/stream.h>

#include <stdlib.h>
#include <string.h>

typedef struct single_value_t {
  PS_FIELDS

  ps_val_t *value;
  ps_stream_status next_status;
} single_value_t;


ps_val_t *single_value_fn(ps_t *base, ps_stream_status status) {
  if (ps_status(base, status)) {
    return NULL;
  }

  single_value_t *s = (single_value_t *)base;
  if (!s->value) {
    ps_status(base, PS_DONE);
    return NULL;
  }

  ps_val_t *val = s->value;
  s->value = NULL;

  return val;
}

void single_value_destroy_fn(ps_handle_t *base) {
  single_value_t *s = (single_value_t *)base;
  if (s->value) {
    ps_destroy(s->value);
  }

  free(base);
}

ps_t *create_single_value(void *data, uint64_t len) {
  if (!data || !len) {
    return NULL;
  }

  single_value_t *s = ps_create_stream(single_value_t, single_value_destroy_fn);

  s->fn = single_value_fn;
  s->value = ps_create_value(ps_val_t, NULL);
  s->value->data = calloc(len + 1, 1);
  memcpy(s->value->data, data, len);
  s->value->len = len;

  return (ps_t *)s;
}
