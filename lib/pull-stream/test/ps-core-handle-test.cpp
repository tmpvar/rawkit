#include <doctest.h>

#include <pull/stream.h>


typedef struct my_duplex_t {
  PS_DUPLEX_FIELDS
} my_duplex_t;

typedef struct my_stream_t {
  PS_FIELDS
} my_stream_t;

typedef struct my_value_t {
  PS_VALUE_FIELDS
} my_value_t;

TEST_CASE("[pull/stream/core] handles - no destructor") {
  // duplex: construction / destruction
  {
    my_duplex_t *d = ps_create_duplex(my_duplex_t, NULL);
    REQUIRE(d != nullptr);
    CHECK(d->handle_type == PS_HANDLE_DUPLEX);
    CHECK(sizeof(*d) == sizeof(my_duplex_t));
    ps_destroy(d);
    REQUIRE(d == nullptr);
  }
  // stream: construction / destruction
  {
    my_stream_t *s = ps_create_stream(my_stream_t, NULL);
    REQUIRE(s != nullptr);
    CHECK(s->handle_type == PS_HANDLE_STREAM);
    CHECK(sizeof(*s) == sizeof(my_stream_t));

    ps_destroy(s);
    REQUIRE(s == nullptr);
  }

  // value: construction / destruction
  {
    my_value_t *v = ps_create_value(my_value_t, NULL);
    v->data = malloc(512);
    REQUIRE(v != nullptr);
    CHECK(v->handle_type == PS_HANDLE_VALUE);
    CHECK(sizeof(*v) == sizeof(my_value_t));
    ps_destroy(v);
    REQUIRE(v == nullptr);
  }
}