#include <stdbool.h>
#include <stdint.h>

#include <pull/stream.h>

#include <uv.h>

typedef struct file_source_t {
  PS_FIELDS

  uv_loop_t *loop;
  uv_fs_t open_req;
  uv_fs_t read_req;
  uv_fs_t close_req;
  uv_file handle;

  ps_val_t *value;

  uint64_t read_buffer_len;
  bool read_pending;
  uv_buf_t read_buffer;
} file_source_t;

void on_source_file_close(uv_fs_t* req) {
  file_source_t* s = (file_source_t*)req->data;
  if (ps_status((ps_t *)s, PS_OK)) {
    return;
  }

  if (s->read_buffer.base != NULL) {
    free(s->read_buffer.base);
    s->read_buffer.base = NULL;
    s->read_buffer.len = 0;
  }

  ps_status((ps_t *)s, PS_DONE);
}

void on_source_file_read(uv_fs_t *req) {
  file_source_t *s = (file_source_t *)req->data;

  if (req->result < 0) {
    ps_status((ps_t *)s, PS_ERR);
    return;
  }

  if (req->result == 0) {
    uv_fs_close(
      s->loop,
      &s->close_req,
      (uv_file)s->handle,
      on_source_file_close
    );
    return;
  }

  s->read_pending = false;

  ps_val_t *val = (ps_val_t *)calloc(sizeof(ps_val_t), 1);
  val->len = req->result;
  val->data = s->read_buffer.base;

  s->read_buffer.base = NULL;
  s->read_buffer.len = 0;
  s->value = val;
}

void request_read(file_source_t *s) {
  if (s->read_pending || s->handle == -1) {
    return;
  }

  s->read_pending = true;
  s->read_buffer.len = s->read_buffer_len;
  s->read_buffer.base = (char *)calloc(s->read_buffer_len + 1, 1);

  if (s->read_buffer.base == NULL) {
    ps_status((ps_t *)s, PS_ERR);
    return;
  }

  s->read_req.data = (void *)s;

  uv_fs_read(
    s->loop,
    &s->read_req,
    s->handle,
    &s->read_buffer,
    1,
    -1,
    on_source_file_read
  );
}

static ps_val_t *file_source_fn(ps_t *base, ps_stream_status status) {
  file_source_t *source = (file_source_t *)base;

  if (ps_status(base, status)) {
    return NULL;
  }

  if (!source->value) {
    request_read(source);
    return NULL;
  }

  ps_val_t *val = source->value;
  source->value = NULL;
  return val;
}

static void on_source_file_open(uv_fs_t *req) {
  file_source_t *s = (file_source_t *)req->data;
  int result = uv_fs_get_result(req);

  if (result < 0) {
    ps_status((ps_t *)s, PS_ERR);
    return;
  }

  s->handle = (uv_file)result;
  request_read(s);
}

ps_t *create_file_source(const char *path, uv_loop_t *loop) {
  if (!path || !loop) {
    return NULL;
  }

  file_source_t *s = (file_source_t *)calloc(sizeof(file_source_t), 1);

  s->fn = file_source_fn;
  s->loop = loop;
  s->read_buffer_len = 32768;
  s->open_req.data = (void *)s;
  s->close_req.data = (void *)s;
  s->read_req.data = (void *)s;
  s->handle = -1;
  uv_fs_open(
    loop,
    &s->open_req,
    path,
    O_RDONLY,
    0,
    on_source_file_open
  );

  return (ps_t *)s;
}
