#include <pull/stream.h>

static ps_val_t *reverser_fn(ps_t *s, ps_stream_status status) {
  if (ps_status(s, status)) {
    return NULL;
  }

  ps_val_t *val = ps_pull(s, PS_OK);
  if (!val || !val->data || !val->len) {
    return NULL;
  }

  uint64_t l = val->len;
  uint64_t half = l / 2;
  uint8_t *data = val->data;

  for (uint64_t i=0; i<half; i++) {
    uint64_t target = l - (i + 1);
    uint8_t tmp = data[target];
    data[target] = data[i];
    data[i] = tmp;
  }

  return val;
}

ps_t *create_reverser() {
  ps_t *s = ps_create_stream(ps_t, NULL);
  s->fn = reverser_fn;
  return s;
}