#include <pull/stream.h>

#include <stdint.h>

typedef struct taker_t {
  PS_CB_FIELDS
  int64_t n;
} taker_t;

static ps_val_t *taker_fn(ps_t *base, ps_status status) {
  taker_t *taker = (taker_t *)base;
  if (taker->n <= 0) {
    handle_status(base, PS_DONE);
    return NULL;
  }

  taker->n--;

  return pull_through(base, status);
}

ps_t *create_taker(int64_t n) {
  taker_t *taker = (taker_t *)calloc(sizeof(taker_t), 1);

  taker->fn = taker_fn;
  taker->n = n;

  return (ps_t *)taker;
}