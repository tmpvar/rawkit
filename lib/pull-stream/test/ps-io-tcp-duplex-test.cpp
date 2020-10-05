#include <doctest.h>

#include <pull/stream.h>
#include <uv.h>

#include <string.h>

#include <vector>

class TCPEchoServer {
  uv_loop_t *loop;
  uv_tcp_t server;
  struct sockaddr_in addr;
  bool error = false;
  std::vector <uv_tcp_t *> clients;
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
      uv_close((uv_handle_t *)&server, NULL);
      for (uv_tcp_t *client : clients) {
        uv_close((uv_handle_t *)client, NULL);
      }
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
        that->clients.push_back(client);
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
          printf("SERVER READ: %s\n", a);
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

  // connection success
  {
    uv_loop_t loop;
    uv_loop_init(&loop);
    const uint16_t port = 57876;
    TCPEchoServer *server = new TCPEchoServer(port, &loop);
    ps_duplex_t *client = create_tcp_client("127.0.0.1", port, &loop);
    REQUIRE(client != nullptr);
    CHECK(client->status == PS_OK);

    int sentinel = 100;
    while (client->status == PS_OK && sentinel --) {
      uv_run(&loop, UV_RUN_NOWAIT);
    }

    CHECK(sentinel <= 0);
    CHECK(client->status == PS_OK);

    delete server;
    ps_destroy(client);
    uv_stop(&loop);
  }

  // send recv
  {
    uv_loop_t loop;
    uv_loop_init(&loop);
    const uint16_t port = 57876;
    TCPEchoServer *server = new TCPEchoServer(port, &loop);
    ps_duplex_t *client = create_tcp_client("127.0.0.1", port, &loop);
    REQUIRE(client != nullptr);
    REQUIRE(client->status == PS_OK);
    REQUIRE(client->sink != nullptr);
    REQUIRE(client->source != nullptr);

    char *src_value = "hello world as seen bounced off of an echo server";
    ps_t *source = create_single_value((void *)src_value, strlen(src_value));
    client->sink->source = source;

    ps_t *limiter = create_taker(1, PS_DONE);
    ps_t *collect = create_collector();
    limiter->source = client->source;
    collect->source = limiter;

    int sentinel = 10000;
    ps_val_t *val = NULL;
    while (!val && sentinel --) {
      uv_run(&loop, UV_RUN_NOWAIT);

      client->sink->fn(client->sink, PS_OK);
      val = collect->fn(collect, PS_OK);
    }

    REQUIRE(val != nullptr);
    CHECK(val->len == strlen(src_value));
    CHECK(strcmp((char *)val->data, src_value) == 0);

    CHECK(sentinel > 0);
    CHECK(client->status == PS_DONE);

    delete server;
    ps_destroy(client);
    uv_stop(&loop);
  }
}
