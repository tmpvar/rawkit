#include <pull/stream.h>

#include <stdlib.h>
#include <string.h>

typedef struct single_value_t {
  PS_FIELDS

  ps_val_t *value;
  ps_status next_status;
} single_value_t;


ps_val_t *single_value_fn(ps_t *base, ps_status status) {
  if (handle_status(base, status)) {
    return NULL;
  }

  single_value_t *s = (single_value_t *)base;
  if (!s->value) {
    handle_status(base, PS_DONE);
    return NULL;
  }

  ps_val_t *val = s->value;
  s->value = NULL;

  return val;
}

void single_value_destroy_fn(ps_t *base) {
  single_value_t *s = (single_value_t *)base;
  if (s->value) {
    ps_val_destroy(s->value);
    s->value = NULL;
  }

  free(base);
}

ps_t *create_single_value(void *data, uint64_t len) {
  if (!data || !len) {
    return NULL;
  }

  single_value_t *s = (single_value_t *)calloc(sizeof(single_value_t), 1);

  s->fn = single_value_fn;
  s->destroy_fn = single_value_destroy_fn;

  s->value = (ps_val_t *)malloc(sizeof(ps_val_t));
  s->value->data = calloc(len + 1, 1);
  memcpy(s->value->data, data, len);
  s->value->len = len;
  s->value->destroy_fn = free;
  return (ps_t *)s;
}
