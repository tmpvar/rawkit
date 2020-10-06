#include <pull/stream.h>

typedef struct tcp_server_t {
  PS_FIELDS

  uv_loop_t *loop;
  uv_tcp_t server;
  struct sockaddr_in addr;
  uv_tcp_t *pending_client;
} tcp_server_t;

static void on_server_close(uv_handle_t* handle) {
  tcp_server_t *s = (tcp_server_t *)handle->data;
  ps_status(s, PS_DONE);
  free(handle);
}

static void on_client_close(uv_handle_t* handle) {
  tcp_server_t *s = (tcp_server_t *)handle->data;
  ps_status(s, PS_DONE);
  free(handle);
}

static ps_val_t *tcp_server_fn(ps_t *base, ps_stream_status status) {
  if (ps_status(base, status)) {
    return NULL;
  }

  tcp_server_t *s = (tcp_server_t *)base;
  if (!s->pending_client) {
    return NULL;
  }

  int r = uv_accept(
    (uv_stream_t *)&s->server,
    (uv_stream_t *)s->pending_client
  );

  if (r) {
    s->pending_client = NULL;
    uv_close(
      (uv_handle_t *)s->pending_client,
      on_client_close
    );
    ps_status(s, PS_DONE);
    return NULL;
  }


  // create a value and return it
  // TODO: we probably want to provide a destroy_fn here..
  ps_val_t *value = ps_create_value(ps_val_t, NULL);

  value->data = create_tcp_client_from_stream(s->pending_client, s->loop);
  value->len = sizeof(uv_tcp_t);
  s->pending_client = NULL;
  return value;
}

static void on_connection(uv_stream_t *server, int status) {
  tcp_server_t *s = (tcp_server_t *)server->data;
  if (!s) {
    printf("FATAL: tcp_server:on_connection: missing link back to server\n");
    return;
  }

  if (status < 0) {
    fprintf(stderr, "New connection error %s\n", uv_strerror(status));
    ps_status(s, PS_ERR);
    return;
  }


  s->pending_client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(s->loop, s->pending_client);
}

ps_t *create_tcp_server(const char *host, uint16_t port, uv_loop_t *loop) {
  // TODO: currently limited to handling one client at a time until
  //       a ringbuffer is used to buffer accepted clients.
  int backlog = 1;

  if (!host || !port || !loop) {
    return NULL;
  }

  struct sockaddr_in addr;
  if (uv_ip4_addr(host, port, &addr)) {
    return NULL;
  }

  tcp_server_t *stream = ps_create_stream(tcp_server_t, NULL);

  stream->fn = tcp_server_fn;
  stream->loop = loop;
  stream->addr = addr;
  uv_tcp_init(loop, &stream->server);
  {
    int r = uv_tcp_bind(
      &stream->server,
      (const struct sockaddr*)&stream->addr,
      0
    );

    if (r) {
      printf("ERROR: server-tcp: failed to bind (%i)\n", r);
      ps_status(stream, PS_ERR);
      return (ps_t *)stream;
    }
  }

  stream->server.data = stream;
  int r = uv_listen(
    (uv_stream_t*)&stream->server,
    backlog,
    on_connection
  );

  if (r) {
    ps_status(stream, PS_ERR);
    return (ps_t *)stream;
  }

  return (ps_t *)stream;
}