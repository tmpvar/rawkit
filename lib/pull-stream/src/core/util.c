#include <pull/stream.h>

ps_status handle_status(ps_t *cb, ps_status status) {
  if (!cb) {
    return PS_ERR;
  }

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
  }

  return status;
}

ps_val_t *pull_through(ps_t* cb, ps_status status) {
  if (handle_status(cb, status)) {
    return NULL;
  }

  if (!cb->source || !cb->source->fn) {
    cb->status = PS_ERR;
    return NULL;
  }

  ps_val_t* v = cb->source->fn(cb->source, status);
  handle_status(cb, cb->source->status);

  return v;
}

void ps_destroy(ps_t *s) {
  if (!s || !s->destroy_fn) {
    free(s);
    return;
  }

  s->destroy_fn(s);
}

void ps_val_destroy(ps_val_t *val) {
  if (!val || !val->destroy_fn) {
    return;
  }

  val->destroy_fn(val->data);
  free(val);
}
