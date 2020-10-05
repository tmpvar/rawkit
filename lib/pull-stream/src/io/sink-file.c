#include <pull/stream.h>
#include <uv.h>

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

typedef struct file_sink_t {
  PS_FIELDS

  uv_loop_t *loop;
  uv_file handle;

  uv_fs_t req;

  bool close_pending;
  ps_val_t *value;
  uv_buf_t write_buffer;

} file_sink_t;

static void on_sink_file_open(uv_fs_t *req) {
  file_sink_t *s = (file_sink_t *)req->data;
  int result = uv_fs_get_result(req);

  if (result < 0 || !s) {
    handle_status((ps_t *)s, PS_ERR);
    return;
  }

  s->handle = (uv_file)result;
}

static void on_sink_file_close(uv_fs_t *req) {
  file_sink_t *s = (file_sink_t *)req->data;
  if (!s) {
    return;
  }

  handle_status((ps_t *)s, PS_DONE);
  s->handle = -1;
}

static void on_sink_file_write(uv_fs_t *req) {
  file_sink_t *sink = (file_sink_t *)req->data;
  ps_destroy(sink->value);
  sink->value = NULL;
}

ps_val_t *file_sink_fn(ps_t *base, ps_status status) {
  if (handle_status(base, status)) {
    return NULL;
  }

  if (!base->source) {
    handle_status(base, PS_ERR);
    return NULL;
  }

  file_sink_t *s = (file_sink_t *)base;

  // not open yet
  if (s->handle == -1) {
    return NULL;
  }

  // still writing the previous packet
  if (s->value) {
    return NULL;
  }

  ps_val_t *val = pull_through(base, PS_OK);

  // handle ERR/DONE
  if (base->status) {
    if (base->status == PS_ERR) {
      ps_destroy(s->value);
      s->value = NULL;
    }

    // there is a value buffered for writing, so maintain this stream's OK status until
    // the data is dequeued
    if (s->value) {
      s->status = PS_OK;
      return NULL;
    }

    if (s->close_pending) {
      return NULL;
    }
    s->close_pending = true;

    uv_fs_close(
      s->loop,
      &s->req,
      (uv_file)s->handle,
      on_sink_file_close
    );

    return NULL;
  }

  s->value = val;

  if (!s->value) {
    return NULL;
  }

  // fire off a write request if we have data and are not currently writing
  s->write_buffer.len = s->value->len;
  s->write_buffer.base = s->value->data;

  uv_fs_write(
    s->loop,
    &s->req,
    s->handle,
    &s->write_buffer,
    1,
    -1,
    on_sink_file_write
  );

  return NULL;
}

ps_t *create_file_sink(const char *path, uv_loop_t *loop) {
  if (!path || !loop) {
    return NULL;
  }

  file_sink_t *sink = (file_sink_t *)calloc(sizeof(file_sink_t), 1);

  sink->loop = loop;
  sink->req.data = (void *)sink;
  sink->fn = file_sink_fn;
  sink->handle = -1;

  uv_fs_open(
    loop,
    &sink->req,
    path,
    O_WRONLY | O_CREAT | O_TRUNC,
    0644,
    on_sink_file_open
  );

  return (ps_t *)sink;
}
