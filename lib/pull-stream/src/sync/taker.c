#include <pull/stream.h>

#include <stdint.h>

typedef struct taker_t {
  PS_FIELDS
  int64_t n;
  ps_stream_status next_status;
} taker_t;

static ps_val_t *taker_fn(ps_t *base, ps_stream_status status) {
  taker_t *taker = (taker_t *)base;
  if (taker->n <= 0) {
    ps_status(base, taker->next_status);
    return NULL;
  }

  taker->n--;

  return ps_pull(base, status);
}

ps_t *create_taker(int64_t n, ps_stream_status status) {
  taker_t *taker = ps_create_stream(taker_t, NULL);

  taker->fn = taker_fn;
  taker->n = n;
  taker->next_status = status;

  return (ps_t *)taker;
}
