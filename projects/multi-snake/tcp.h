#include <rawkit/rawkit.h>
#include <pull/stream.h>
#include <stb_sb.h>
#include <uv.h>

struct BufferedSource {
  PS_FIELDS

  ps_val_t **pending = nullptr;
  i32 read_head = 0;
  static BufferedSource *create(ps_t *sink) {
    printf("here?\n");
    if (!sink) {
      printf("BufferedSource::create: invalid sink\n");
      return nullptr;
    }
    printf("valid sink\n");

    BufferedSource *source = ps_create_stream(BufferedSource, nullptr);
    source->fn = BufferedSource::source_fn;
    sink->source = (ps_t *)source;
    return source;
  }

  static void destroy(ps_handle_t *s) {
    if (!s) {
      return;
    }

    BufferedSource *src = (BufferedSource *)s;

    // free all of the pending values
    {
      u32 c = sb_count(src->pending);
      for(u32 i=0; i<c; i++) {
        if (src->pending[i]) {
          ps_destroy(src->pending[i]);
        }
      }
      sb_free(src->pending);
    }
  }

  static ps_val_t *source_fn(ps_t *base, ps_stream_status status) {
    BufferedSource *src = (BufferedSource *)base;
    if (!src) {
      return nullptr;
    }

    src->read_head++;

    if (sb_count(src->pending) <= src->read_head) {
      sb_reset(src->pending);
      src->read_head = -1;
      return nullptr;
    }

    ps_val_t *ret = src->pending[src->read_head];
    // we no longer own this data.
    src->pending[src->read_head] = nullptr;
    return ret;
  }

  void append(u8 *data, u32 len) {
    ps_val_t *val = ps_create_value(ps_val_t, nullptr);
    val->data = data;
    val->len = len;
    sb_push(this->pending, val);
  }

};

struct TCPClient {
  ps_duplex_t *stream;
  uv_loop_t *loop = nullptr;
  BufferedSource *src;

  TCPClient(const char *addr, u32 port, uv_loop_t *loop = nullptr) {
    this->loop = loop ? loop : uv_default_loop();
    this->stream = create_tcp_client(addr, port, this->loop);
    this->src = BufferedSource::create(this->stream->sink);
  }

  ~TCPClient() {
    ps_destroy(this->stream);
    ps_destroy(this->src);
  }

  ps_val_t *read() {
    return ps_pull(this->stream->source, PS_OK);
  }

  void write_copy(const u8 *src_bytes, const u32 byte_count) {
    u8 *bytes = (u8 *)malloc(byte_count);
    if (!bytes) {
      printf("TCPClient::write_copy: could not alloc bytes copy\n");
      return;
    }
    memcpy(bytes, src_bytes, byte_count);
    this->src->append(bytes, byte_count);

    ps_pull(this->stream->sink, PS_OK);
  }
};



struct TCPServer {
  uv_loop_t *loop = nullptr;
  uv_tcp_t server;
  struct sockaddr_in addr;
  bool error = false;
  uv_tcp_t **clients = nullptr;

  u64 bytes_sent = 0;
  u64 bytes_recv = 0;

  typedef struct {
    uv_write_t req;
    uv_buf_t buf;
  } write_req_t;

  public:
    TCPServer(uint16_t port) {
      this->loop = uv_default_loop();
      uv_tcp_init(loop, &server);
      uv_ip4_addr("0.0.0.0", port, &addr);
      uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
      this->server.data = this;
      int r = uv_listen(
        (uv_stream_t*)&server,
        5, // backlog
        TCPServer::on_new_connection
      );

      if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        error = true;
      }
    }

    ~TCPServer() {
      uv_close((uv_handle_t *)&server, nullptr);
      u32 c = sb_count(this->clients);

      for (u32 i=0; i<c; i++){
        uv_close((uv_handle_t *)this->clients[i], nullptr);
        this->clients[i] = nullptr;
      }
      sb_free(this->clients);
      this->clients = nullptr;
    }

    static void on_new_connection(uv_stream_t *server, int status) {
      if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
      }

      printf("new connection!\n");

      TCPServer *that = (TCPServer *)server->data;

      uv_tcp_t *client = (uv_tcp_t*) calloc(sizeof(uv_tcp_t), 1);
      uv_tcp_init(that->loop, client);

      if (uv_accept(server, (uv_stream_t*) client) == 0) {
        client->data = (void *)that;
        sb_push(that->clients, client);
        uv_read_start((uv_stream_t*) client, TCPServer::alloc_buffer, TCPServer::read);
      } else {
        uv_close((uv_handle_t*) client, TCPServer::on_close);
      }
    }

    static void alloc_buffer(uv_handle_t *, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      #ifdef WIN32
        buf->len = (ULONG)suggested_size;
      #else
        buf->len = suggested_size;
      #endif
    }

    static void on_close(uv_handle_t* handle) {
      if (!handle || !handle->data) {
        return;
      }

      TCPServer *that = (TCPServer *)handle->data;
      if (that->clients == nullptr) {
        printf("that->clients == nullptr\n");
        return;
      }
      u32 c = sb_count(that->clients);
      if (c == 1) {
        sb_reset(that->clients);
      } else {
        for (u32 i=0; i<c; i++) {
          if (that->clients[i] == (uv_tcp_t *)handle) {
            that->clients[i] = sb_last(that->clients);
            stb__sbn(that->clients)--;
            break;
          }
        }
      }

      free(handle);
    }

    static void read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
      TCPServer *that = (TCPServer *)client->data;
      if (nread > 0) {
        write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, static_cast<unsigned int>(nread));
        that->bytes_recv += nread;

        uv_write((uv_write_t*) req, client, &req->buf, 1, TCPServer::write);
        return;
      }
      if (nread < 0) {
        if (nread != UV_EOF) {
          fprintf(stderr, "Read error %s\n", uv_err_name(static_cast<int>(nread)));
        }
        uv_close((uv_handle_t*) client, TCPServer::on_close);
      }

      free(buf->base);
    }

    static void write(uv_write_t *req, int status) {
      write_req_t *wr = (write_req_t*) req;
      if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
      } else {

        TCPServer *that = (TCPServer *)wr->req.handle->data;
        that->bytes_sent += wr->buf.len;
      }
      free(wr->buf.base);
      free(wr);
    }
};