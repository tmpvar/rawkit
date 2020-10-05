#include <doctest.h>

#include <pull/stream.h>

#include <string.h>

TEST_CASE("[pull/stream] single value source stream") {

  // null if data is null or len is 0
  {
    REQUIRE(create_single_value(NULL, 0) == nullptr);
    REQUIRE(create_single_value(NULL, 1) == nullptr);
    const char *str = "asdf";
    REQUIRE(create_single_value((void *)str, 0) == nullptr);
  }

  // print a value one time
  {
    const char *str = "hello world";
    const uint64_t len = strlen(str);
    ps_t *source = create_single_value((void *)str, len);
    REQUIRE(source != nullptr);
    CHECK(source->status == PS_OK);

    // read the value
    {
      ps_val_t *val = source->fn(source, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(val->len == len);
      // ensure single value makes a heap allocated copy. This is for consistency
      // in ownership / destruction.
      CHECK(val->data != str);
      CHECK(source->status == PS_OK);

      CHECK(strcmp((char *)val->data, str) == 0);

      ps_destroy(val);
    }

    // read again (DONE)
    {
      ps_val_t *val = source->fn(source, PS_OK);
      REQUIRE(val == nullptr);
      CHECK(source->status == PS_DONE);
    }

    ps_destroy(source);
  }
}
