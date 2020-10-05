#include <doctest.h>

#include <pull/stream.h>
#include <uv.h>

#include <string.h>

class TCPEchoServer;

TEST_CASE("[pull/stream/io] duplex tcp") {
  // fail to construct when provided invalid args
  {
    uv_loop_t loop;
    const char *addr = "1.2.3.4";
    REQUIRE(create_tcp_client(NULL, 123, NULL) == nullptr);
    REQUIRE(create_tcp_client(NULL, 123, &loop) == nullptr);
    REQUIRE(create_tcp_client(addr, 123, NULL) == nullptr);
  }

  // attempt connection on a port that is not listening
  {
    uv_loop_t loop;
    uv_loop_init(&loop);

    ps_duplex_t *client = create_tcp_client("127.0.0.1", 65000, &loop);
    REQUIRE(client != nullptr);
    CHECK(client->status == PS_OK);

    int sentinel = 1000;
    while (client->status == PS_OK && sentinel --) {
      uv_run(&loop, UV_RUN_NOWAIT);
    }

    CHECK(sentinel > 0);
    CHECK(client->status == PS_ERR);
  }
}

class TCPEchoServer {
  uv_loop_t *loop;
  uv_tcp_t server;
  struct sockaddr_in addr;
  bool error = false;

  typedef struct {
    uv_write_t req;
    uv_buf_t buf;
  } write_req_t;

  public:
    TCPEchoServer(uint16_t port, uv_loop_t *loop) {
      this->loop = loop;
      uv_tcp_init(loop, &server);
      uv_ip4_addr("127.0.0.1", port, &addr);
      uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
      server.data = this;
      int r = uv_listen(
        (uv_stream_t*)&server,
        5, // backlog
        TCPEchoServer::on_new_connection
      );

      if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        error = true;
      }
    }

    ~TCPEchoServer() {

    }


    static void on_new_connection(uv_stream_t *server, int status) {
      if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
      }

      printf("SERVER: new connection!\n");
      TCPEchoServer *that = (TCPEchoServer *)server->data;

      uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
      uv_tcp_init(that->loop, client);
      if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_read_start((uv_stream_t*) client, TCPEchoServer::alloc_buffer, TCPEchoServer::echo_read);
      } else {
        uv_close((uv_handle_t*) client, TCPEchoServer::on_close);
      }
    }

    static void alloc_buffer(uv_handle_t *, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      buf->len = (ULONG)suggested_size;
    }

    static void on_close(uv_handle_t* handle) {
      free(handle);
    }

    static void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
      if (nread > 0) {
        write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, static_cast<unsigned int>(nread));

        {
          char *a = (char *)calloc(nread + 1, 1);
          memcpy(a, buf->base, nread);
          REQUIRE(strstr(a, "FIND THIS STRING") != nullptr);
          free(a);
        }

        uv_write((uv_write_t*) req, client, &req->buf, 1, TCPEchoServer::echo_write);
        return;
      }
      if (nread < 0) {
        if (nread != UV_EOF) {
          fprintf(stderr, "Read error %s\n", uv_err_name(static_cast<int>(nread)));
        }
        uv_close((uv_handle_t*) client, TCPEchoServer::on_close);
      }

      free(buf->base);
    }

    static void echo_write(uv_write_t *req, int status) {
      if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
      }

      write_req_t *wr = (write_req_t*) req;
      free(wr->buf.base);
      free(wr);
    }
};
