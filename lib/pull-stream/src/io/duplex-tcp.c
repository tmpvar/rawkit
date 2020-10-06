
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
  s->ready = true;
  if (status < 0) {
    s->status = PS_ERR;
    return;
  }
}

ps_duplex_t *create_tcp_client(const char *addr, uint16_t port, uv_loop_t *loop) {
  if (!addr || !loop) {
    return NULL;
  }

  tcp_t *duplex = ps_create_stream(tcp_t, NULL);
  // setup the source stream
  {
    ps_duplex_io_source_t *source = NULL;
    source = (ps_duplex_io_source_t *)ps_create_stream(
      ps_duplex_io_source_t,
      NULL
    );

    if (!source) {
      ps_destroy(duplex);
      return NULL;
    }

    source->fn = ps_uv_source_fn;
    source->duplex = (ps_duplex_t *)duplex;
    duplex->source = (ps_t *)source;
  }

  {
    // setup the sink stream
    ps_duplex_io_sink_t *sink = NULL;
    sink = (ps_duplex_io_sink_t *)ps_create_stream(
      ps_duplex_io_sink_t,
      NULL
    );

    if (!sink) {
      ps_destroy(duplex);
      return NULL;
    }

    sink->fn = ps_uv_sink_fn;
    sink->duplex = (ps_duplex_t *)duplex;
    duplex->sink = (ps_t *)sink;
  }


  uv_ip4_addr(addr, port, &duplex->dest);
  duplex->loop = loop;
  duplex->stream = (uv_stream_t *)calloc(sizeof(uv_tcp_t), 1);
  uv_tcp_init(loop, (uv_tcp_t *)duplex->stream);
  duplex->stream->data = (void*)duplex;

  // begin connection
  duplex->connect_req.data = (void *)duplex;
  int r = uv_tcp_connect(
    &duplex->connect_req,
    (uv_tcp_t *)duplex->stream,
    (const struct sockaddr *)&duplex->dest,
    on_connect_cb
  );

  if (r < 0) {
    duplex->status = PS_ERR;
  }

  return (ps_duplex_t *)duplex;
}

ps_duplex_t *create_tcp_client_from_stream(uv_tcp_t *client, uv_loop_t *loop) {
  tcp_t *duplex = ps_create_stream(tcp_t, NULL);
  // setup the source stream
  {
    ps_duplex_io_source_t *source = NULL;
    source = (ps_duplex_io_source_t *)ps_create_stream(
      ps_duplex_io_source_t,
      NULL
    );

    if (!source) {
      ps_destroy(duplex);
      return NULL;
    }

    source->fn = ps_uv_source_fn;
    source->duplex = (ps_duplex_t *)duplex;
    duplex->source = (ps_t *)source;
  }

  {
    // setup the sink stream
    ps_duplex_io_sink_t *sink = NULL;
    sink = (ps_duplex_io_sink_t *)ps_create_stream(
      ps_duplex_io_sink_t,
      NULL
    );

    if (!sink) {
      ps_destroy(duplex);
      return NULL;
    }

    sink->fn = ps_uv_sink_fn;
    sink->duplex = (ps_duplex_t *)duplex;
    duplex->sink = (ps_t *)sink;
  }

  duplex->loop = loop;
  duplex->stream = (uv_stream_t *)client;
  duplex->stream->data = (void*)duplex;
  duplex->ready = true;

  uv_read_start(
    (uv_stream_t*) client,
    ps_uv_alloc_cb,
    ps_uv_read_cb
  );

  return (ps_duplex_t *)duplex;
}