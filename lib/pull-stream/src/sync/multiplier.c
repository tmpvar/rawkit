#include <pull/stream.h>

#include <string.h>

// multiplier (uint64)
typedef struct multiplier_t {
  PS_FIELDS
  uint64_t scale;
  uint64_t value;
} multiplier_t;

static ps_val_t *multiplier_cb(ps_t *base, ps_stream_status status) {
  ps_val_t *input = ps__pull_from_source(base, status);
  if (!input) {
    return NULL;
  }

  // invalid value data or size
  if (!input->data || input->len < sizeof(uint64_t)) {
    ps_status(base, PS_ERR);
    ps_destroy(input);
    return NULL;
  }

  multiplier_t *mult = (multiplier_t *)base;
  mult->value = (*(uint64_t *)input->data) * mult->scale;
  ps_destroy(input);

  ps_val_t *output = ps_create_value(ps_val_t, NULL);
  output->len = sizeof(mult->value);
  output->data = calloc(output->len, 1);

  memcpy(output->data, (void *)&mult->value, output->len);

  return output;
}

ps_t *create_multiplier(uint64_t scale) {
  multiplier_t *s = ps_create_stream(multiplier_t, NULL);

  s->scale = scale;
  s->fn = multiplier_cb;

  return (ps_t *)s;
}
