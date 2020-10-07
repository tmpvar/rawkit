#include <pull/stream.h>

#include <string.h>

static ps_val_t *user_value_fn(ps_t *base, ps_stream_status status) {
  ps_user_value_t* s = (ps_user_value_t*)base;

  switch (ps_status(base, status)) {
    case PS_OK: {
      ps_val_t* val = s->value;
      s->value = NULL;
      return val;
    }

    case PS_ERR: {
      ps_destroy(s->value);
      return NULL;
    }

    case PS_DONE: {
      if (s->value) {
        ps_val_t* val = s->value;
        s->value = NULL;
        return val;
      }
      return NULL;
    }
  }

  return NULL;
}


void ps_user_value_from_str(ps_user_value_t *s, const char *str) {
  if (!s || !str) {
    return;
  }

  if (s->value) {
    ps_destroy(s->value);
    s->value = NULL;
  }

  s->value = ps_create_value(ps_val_t, NULL);
  if (!s->value) {
    ps_status(s, PS_ERR);
    return;
  }

  s->value->len = strlen(str);
  s->value->data = calloc(s->value->len + 1, 1);
  if (!s->value->data) {
    ps_status(s, PS_ERR);
    ps_destroy(s->value);
    s->value = NULL;
    return;
  }

  memcpy(s->value->data, str, s->value->len);
}

ps_t *create_user_value() {
  ps_user_value_t *s = ps_create_stream(ps_user_value_t, NULL);
  s->fn = user_value_fn;
  return (ps_t *)s;
}