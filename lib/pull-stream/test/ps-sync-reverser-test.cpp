#include <doctest.h>
#include <pull/stream.h>

#include <string.h>


TEST_CASE("[pull/stream] reverser stream") {
  {
    const char *str = "hello";
    ps_t *v = create_single_value(str, strlen(str));
    ps_t *r = create_reverser();

    REQUIRE(r != nullptr);
    REQUIRE(v != nullptr);

    r->source = v;
    ps_val_t *val = r->fn(r, PS_OK);
    REQUIRE(val != nullptr);
    CHECK(strcmp("olleh", (char *)val->data) == 0);

    ps_destroy(val);
    ps_destroy(v);
    ps_destroy(r);
  }

  {
    const char* str = "hello!";
    ps_t* v = create_single_value(str, strlen(str));
    ps_t* r = create_reverser();

    REQUIRE(r != nullptr);
    REQUIRE(v != nullptr);

    r->source = v;
    ps_val_t* val = r->fn(r, PS_OK);
    REQUIRE(val != nullptr);
    CHECK(strcmp("!olleh", (char*)val->data) == 0);

    ps_destroy(val);
    ps_destroy(v);
    ps_destroy(r);
  }
}