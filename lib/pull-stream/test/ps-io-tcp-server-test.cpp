#include <doctest.h>

#include <pull/stream.h>
#include <string.h>

#include <vector>

TEST_CASE("[pull/stream/io] tcp server") {
  // returns null on invalid input
  {
    REQUIRE(create_tcp_server(NULL, 0, NULL) == nullptr);

    uv_loop_t loop;
    REQUIRE(create_tcp_server(NULL, 0, &loop) == nullptr);
    REQUIRE(create_tcp_server(NULL, 1, &loop) == nullptr);
    REQUIRE(create_tcp_server("hello", 1, NULL) == nullptr);
    REQUIRE(create_tcp_server("hello", 0, &loop) == nullptr);
    REQUIRE(create_tcp_server("hello", 9000, &loop) == nullptr);
  }

  // create a valid echo server
  {
    uv_loop_t loop;
    uv_loop_init(&loop);
    uint16_t port = 56563;
    ps_t *server = create_tcp_server("0.0.0.0", port, &loop);
    REQUIRE(server != nullptr);
    CHECK(server->status == PS_OK);

    ps_duplex_t *client = create_tcp_client("127.0.0.1", port, &loop);

    // send a single packet to the server
    const char *str = "bounce me off of an echo server";
    ps_t *client_payload = create_single_value((void *)str, strlen(str));
    client->sink->source = client_payload;

    // collect a single packet on the client's source
    ps_t *client_taker = create_taker(1, PS_DONE);
    ps_t *client_collector = create_collector();
    client_taker->source = client->source;
    client_collector->source = client_taker;

    std::vector<ps_duplex_t *> server_clients;

    ps_val_t *collected_val = NULL;
    while (collected_val == NULL) {
      uv_run(&loop, UV_RUN_NOWAIT);

      // service the server
      {
        ps_val_t *val = server->fn(server, PS_OK);
        REQUIRE(server->status == PS_OK);
        if (val && val->data) {
          ps_duplex_t *server_client = (ps_duplex_t *)val->data;
          server_client->sink->source = server_client->source;
          server_clients.push_back(server_client);
        }

        for (ps_duplex_t *server_client : server_clients) {
          server_client->sink->fn(server_client->sink, PS_OK);
        }
      }

      // service the client
      {
        client->sink->fn(client->sink, PS_OK);
        collected_val = client_collector->fn(client_collector, PS_OK);
      }
    }

    REQUIRE(client->status == PS_DONE);
    REQUIRE(collected_val != nullptr);

    CHECK(strcmp((char *)collected_val->data, str) == 0);

    for (ps_duplex_t *server_client : server_clients) {
      ps_destroy(server_client);
    }
    ps_destroy(client);
    ps_destroy(server);

    uv_stop(&loop);
  }
}