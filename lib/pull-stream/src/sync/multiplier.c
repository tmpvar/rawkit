#include <pull/stream.h>

#include <stdint.h>

// multiplier (uint64)
typedef struct multiplier_t {
  PS_CB_FIELDS
  uint64_t scale;
  uint64_t value;
} multiplier_t;

static ps_value_t *multiplier_cb(ps_cb_t *base, ps_status status) {
  ps_value_t *input = pull_through(base, status);

  if (!input) {
    return NULL;
  }

  // invalid value data or size
  if (!input->data || input->len < sizeof(uint64_t)) {
    handle_status(base, PS_ERR);
    free(input);
    return NULL;
  }

  multiplier_t *mult = (multiplier_t *)base;

  uint64_t iv = *((uint64_t *)input->data);
  mult->value = iv * mult->scale;

  ps_value_t *output = (ps_value_t *)calloc(sizeof(ps_value_t), 1);
  output->data = (void *)&mult->value;
  output->len = sizeof(mult->value);

  free(input);

  return output;
}

ps_cb_t *create_multiplier(uint64_t scale) {
  multiplier_t *cb = (multiplier_t *)calloc(sizeof(multiplier_t), 1);
  cb->scale = scale;
  cb->fn = multiplier_cb;
  return (ps_cb_t *)cb;
}
