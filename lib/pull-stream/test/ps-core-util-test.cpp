#include <doctest.h>

#include <pull/stream.h>

#include <string.h>

TEST_CASE("[pull/stream/core] ps_status") {

  // invalid input
  {
    CHECK(_ps_status(NULL, PS_OK) == PS_ERR);

    // invalid handle type
    {
      ps_handle_t h = {
        PS_HANDLE_NONE,
        NULL
      };
      CHECK(ps_status(&h, PS_OK) == PS_ERR);
    }

    // value handles have no explicit or directly computable status
    {
      ps_val_t *val = ps_create_value(ps_val_t, NULL);
      REQUIRE(val != nullptr);
      CHECK(ps_status(val, PS_OK) == PS_ERR);
      ps_destroy(val);
    }

    // duplex errors if no sink & source
    {
      ps_duplex_t *d = ps_create_duplex(ps_duplex_t, NULL);
      REQUIRE(d != nullptr);
      CHECK(ps_status(d, PS_OK) == PS_ERR);
      CHECK(d->status == PS_ERR);
      ps_destroy(d);
    }

    // duplex errors if no sink
    {
      ps_duplex_t *d = ps_create_duplex(ps_duplex_t, NULL);
      d->source = ps_create_stream(ps_t, NULL);
      REQUIRE(d != nullptr);
      CHECK(ps_status(d, PS_OK) == PS_ERR);
      CHECK(d->status == PS_ERR);
      ps_destroy(d);
    }

    // duplex errors if no source
    {
      ps_duplex_t *d = ps_create_duplex(ps_duplex_t, NULL);
      d->sink = ps_create_stream(ps_t, NULL);
      REQUIRE(d != nullptr);
      CHECK(ps_status(d, PS_OK) == PS_ERR);
      CHECK(d->status == PS_ERR);
      ps_destroy(d);
    }
  }

  // duplex stream
  {
    ps_duplex_t *d = ps_create_duplex(ps_duplex_t, NULL);
    d->sink = ps_create_stream(ps_t, NULL);
    d->source = ps_create_stream(ps_t, NULL);
    REQUIRE(d != nullptr);
    CHECK(ps_status(d, PS_OK) == PS_OK);
    CHECK(d->status == PS_OK);
    ps_destroy(d);
  }

  // normal stream
  {
    ps_t *s = ps_create_stream(ps_t, NULL);
    REQUIRE(s != nullptr);
    CHECK(ps_status(s, PS_OK) == PS_OK);
    CHECK(s->status == PS_OK);
  }
}

TEST_CASE("[pull/stream/core] ps_pipeline") {
  // invalid arguments
  {
    REQUIRE(ps_pipeline() == nullptr);
    REQUIRE(ps_pipeline(NULL) == nullptr);
    REQUIRE(ps_pipeline(NULL, NULL) == nullptr);
    REQUIRE(ps_pipeline(NULL, NULL, NULL) == nullptr);
  }

  // valid pipeline
  {
    ps_t *s = ps_pipeline(
      create_single_value((void *)"hello", 5),
      create_nooper(),
      create_reverser(),
      create_taker(1, PS_DONE)
    );

    REQUIRE(s != nullptr);

    ps_val_t *val = ps_pull(s, PS_OK);
    REQUIRE(val != nullptr);
    CHECK(val->len == 5);
    CHECK(strcmp((char *)val->data, "olleh") == 0);
  }


}
