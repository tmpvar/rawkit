#include <pull/stream.h>

static inline ps_stream_status ps_status_duplex(ps_duplex_t *d, ps_stream_status status) {
  if (!d->source || !d->sink) {
    d->status = PS_ERR;
    return PS_ERR;
  }

  ps_stream_status source_status = ps_status(d->source, status);
  ps_stream_status sink_status = ps_status(d->sink, status);

  if (source_status == PS_ERR) {
    return ps_status(d->sink, status);
  }

  if (sink_status == PS_ERR) {
    return ps_status(d->source, status);
  }

  if (sink_status == PS_DONE && source_status == PS_DONE) {
    return PS_DONE;
  }

  return status;
}

static inline ps_stream_status ps_status_stream(ps_t *s, ps_stream_status status) {
  if (s->status) {
    return s->status;
  }

  if (s->source && s->source->status) {
    s->status = s->source->status;
  }

  // incoming DONE/ERR
  if (status) {
    s->status = status;

    // push the DONE/ERR up through the chain, each stream is responsible
    // for setting the status freeing any pending data.
    if (s->source) {
      s->source->fn(s->source, status);
    }
  }

  return status;
}

ps_stream_status _ps_status(ps_handle_t *h, ps_stream_status status) {
  if (!h) {
    return PS_ERR;
  }

  switch (h->handle_type) {
    case PS_HANDLE_DUPLEX:
      return ps_status_duplex((ps_duplex_t *)h, status);

    case PS_HANDLE_STREAM:
      return ps_status_stream((ps_t *)h, status);


    // VALUE and NONE do not have explicit status so calling status on
    // these items always results in error.
    default:
      return PS_ERR;
  }
}


// NOTE: this is used internally for pulling values into a sink
// TODO: rename this - it used to be called pull_through 
ps_val_t *ps_pull(ps_t* s, ps_stream_status status) {
  if (ps_status(s, status)) {
    return NULL;
  }

  if (!s->source || !s->source->fn) {
    s->status = PS_ERR;
    return NULL;
  }

  ps_val_t* v = s->source->fn(s->source, status);
  ps_status(s, s->source->status);

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
