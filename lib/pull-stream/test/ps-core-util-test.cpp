#include <doctest.h>

#include <pull/stream.h>


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