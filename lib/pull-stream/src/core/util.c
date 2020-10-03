#include <pull/stream.h>

ps_status handle_status(ps_cb_t *cb, ps_status status) {
  if (cb->status) {
    return cb->status;
  }

  if (cb->source && cb->source->status) {
    cb->status = cb->source->status;
  }

  if (status) {
    cb->status = status;

    if (cb->source) {
      cb->source->fn(cb->source, status);
    }

    return status;
  }

  return status;
}

ps_value_t *pull_through(ps_cb_t* cb, ps_status status) {
  if (handle_status(cb, status)) {
    return NULL;
  }

  if (!cb->source || !cb->source->fn) {
    cb->status = PS_ERR;
    return NULL;
  }

  ps_value_t* v = cb->source->fn(cb->source, status);
  handle_status(cb, cb->source->status);

  return v;
}