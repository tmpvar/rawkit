#include <pull/stream.h>

ps_stream_status ps_status(ps_t *cb, ps_stream_status status) {
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

ps_val_t *ps_pull(ps_t* cb, ps_stream_status status) {
  if (ps_status(cb, status)) {
    return NULL;
  }

  if (!cb->source || !cb->source->fn) {
    cb->status = PS_ERR;
    return NULL;
  }

  ps_val_t* v = cb->source->fn(cb->source, status);
  ps_status(cb, cb->source->status);

  return v;
}

void _ps_destroy(ps_handle_t **p) {
  ps_handle_t *s = *p;

  if (!s) {
    return;
  }

  *p = NULL;

  if (s->handle_destroy_fn) {
    s->handle_destroy_fn(s);
    return;
  }

  switch (s->handle_type) {
    case PS_HANDLE_DUPLEX: {
      ps_duplex_t *duplex = (ps_duplex_t *)s;
      ps_destroy(duplex->source);
      ps_destroy(duplex->sink);

      free(duplex);
      break;
    }

    case PS_HANDLE_STREAM: {
      free(s);
      break;
    }

    case PS_HANDLE_VALUE: {
      ps_val_t *value = (ps_val_t *)s;

      if (value->data) {
        free(value->data);
        value->data = NULL;
        free(value);
      }
    }
  }
}

ps_handle_t *_ps_create(uint64_t size, ps_handle_type type, ps_destroy_fn destroy_fn) {
  ps_handle_t *h = (ps_handle_t *)calloc(size, 1);
  if (!h) {
    return NULL;
  }

  h->handle_type = type;
  h->handle_destroy_fn = destroy_fn;

  return h;
}
