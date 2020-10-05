
#include <pull/stream.h>

#include <stdbool.h>

typedef struct {
  PS_DUPLEX_IO_FIELDS

  struct sockaddr_in dest;
  bool connect_pending;
  uv_connect_t connect_req;
  uv_tcp_t close_req;
} tcp_t;

static void on_connect_cb(uv_connect_t* req, int status) {
  if (req == NULL || req->data == NULL) {
    return;
  }

  tcp_t *s = (tcp_t *)req->data;
  s->connect_pending = false;
  if (status < 0) {
    s->status = PS_ERR;
    return;
  }
}

ps_duplex_t *create_tcp_client(const char *addr, uint16_t port, uv_loop_t *loop) {
  if (!addr || !loop) {
    return NULL;
  }

  tcp_t *s = (tcp_t *)calloc(sizeof(tcp_t), 1);

  uv_ip4_addr(addr, port, &s->dest);
  s->loop = loop;
  s->stream = (uv_stream_t *)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, (uv_tcp_t *)s->stream);
  s->stream->data = (void*)s;

  // begin connection
  s->connect_req.data = (void *)s;
  int r = uv_tcp_connect(
    &s->connect_req,
    (uv_tcp_t *)s->stream,
    (const struct sockaddr *)&s->dest,
    on_connect_cb
  );

  if (r < 0) {
    s->status = PS_ERR;
  }

  return (ps_duplex_t *)s;
}