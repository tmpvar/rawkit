#include <pull/stream.h>

#include <stdint.h>

// multiplier (uint64)
typedef struct multiplier_t {
  PS_CB_FIELDS
  uint64_t scale;
  uint64_t value;
} multiplier_t;

static ps_val_t *multiplier_cb(ps_t *base, ps_status status) {
  ps_val_t *input = pull_through(base, status);
  if (!input) {
    return NULL;
  }

  // invalid value data or size
  if (!input->data || input->len < sizeof(uint64_t)) {
    handle_status(base, PS_ERR);
    ps_val_destroy(input);
    return NULL;
  }

  multiplier_t *mult = (multiplier_t *)base;
  mult->value = (*(uint64_t *)input->data) * mult->scale;
  ps_val_destroy(input);

  ps_val_t *output = (ps_val_t *)calloc(sizeof(ps_val_t), 1);
  output->data = (void *)&mult->value;
  output->len = sizeof(mult->value);

  return output;
}

ps_t *create_multiplier(uint64_t scale) {
  multiplier_t *cb = (multiplier_t *)calloc(sizeof(multiplier_t), 1);
  cb->scale = scale;
  cb->fn = multiplier_cb;
  return (ps_t *)cb;
}
