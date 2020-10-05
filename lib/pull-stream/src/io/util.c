#include <pull/io.h>

void ps_uv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  buf->base = (char*)calloc(suggested_size, 1);
  buf->len = suggested_size;
}

void ps_uv_read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
  ps_duplex_io_t *duplex = (ps_duplex_io_t *)handle->data;
  if (duplex == NULL || ps_status(duplex, PS_OK)) {
    return;
  }

  if (duplex->source == NULL) {
    ps_status(duplex, PS_ERR);
    return;
  }

  uv_read_stop(handle);
  ps_duplex_io_source_t *source = (ps_duplex_io_source_t *)duplex->source;

  // if we the previous read did not reset value then there is a bug
  if (source->value != NULL) {
    printf("FATAL: io duplex source did not clear value before next read\n");
    abort();
    return;
  }

  source->value = ps_create_value(ps_val_t, NULL);
  source->value->data = buf->base;
  source->value->len = nread;

  source->read_pending = false;
}

void ps_uv_write_cb(uv_write_t *base, int status) {
  if (base == NULL) {
    return;
  }

  ps_io_write_req_t *req = (ps_io_write_req_t *)base;
  ps_duplex_io_sink_t *sink = (ps_duplex_io_sink_t *)req->req.data;


  sink->write_pending = false;

  free(req->buf.base);
  req->buf.base = NULL;
  free(req);

  if (status < 0) {
    ps_status(sink, PS_ERR);
    
  }
}

void ps_uv_write(ps_duplex_io_t *duplex, ps_val_t *value) {
  if (!duplex) {
    return;
  }

  if (!value || !duplex->sink || !duplex->stream) {
    ps_status(duplex, PS_ERR);
    return;
  }

  ps_duplex_io_sink_t *sink = (ps_duplex_io_sink_t *)duplex->sink;

  // peform the actual write
  sink->write_pending = true;

  ps_io_write_req_t *req = (ps_io_write_req_t *)calloc(
    sizeof(ps_io_write_req_t),
    1
  );

  if (!req) {
    ps_status(duplex, PS_ERR);
    return;
  }

  req->req.data = (void *)sink;
  req->buf.base = value->data;
  req->buf.len = value->len;
  req->val = value;

  int r = uv_write(
    (uv_write_t *)req,
    duplex->stream,
    &req->buf,
    1,
    ps_uv_write_cb
  );

  if (r < 0) {
    ps_status(sink, PS_ERR);
  }
}


ps_val_t *ps_uv_sink_fn(ps_t *base, ps_stream_status status) {
 if (ps_status(base, status)) {
    return NULL;
  }

  if (!base->source) {
    ps_status(base, PS_ERR);
    return NULL;
  }

  ps_duplex_io_sink_t *s = (ps_duplex_io_sink_t *)base;
  ps_duplex_io_t *duplex = (ps_duplex_io_t *)s->duplex;

  // not open yet
  if (!duplex->ready) {
    return NULL;
  }

  // still writing the previous packet
  if (s->write_pending) {
    return NULL;
  }

  ps_val_t *val = ps_pull(base, PS_OK);

  if (base->status) {
    return NULL;
  }

  ps_uv_write(duplex, val);

  return NULL;
}


ps_val_t *ps_uv_source_fn(ps_t *base, ps_stream_status status) {
  if (ps_status(base, status)) {
    return NULL;
  }

  ps_duplex_io_source_t *source = (ps_duplex_io_source_t *)base;
  ps_duplex_io_t *duplex = (ps_duplex_io_t *)source->duplex;

  // waiting for connection
  if (!duplex->ready) {
    return NULL;
  }

  if (!source->read_pending) {
    source->read_pending = true;
    uv_read_start(
      duplex->stream,
      ps_uv_alloc_cb,
      ps_uv_read_cb
    );
  }

  ps_val_t *val = source->value;
  source->value = NULL;
  return val;
}