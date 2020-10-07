#include <doctest.h>

#include <pull/stream.h>

#include <string.h>

TEST_CASE("[pull/stream] splitter through stream") {
  // invalid creation args
  {
    REQUIRE(create_splitter(0, NULL) == nullptr);
    REQUIRE(create_splitter(1024, NULL) == nullptr);

    const char *str = "hello";
    REQUIRE(create_splitter(0, (const uint8_t *)str) == nullptr);
  }

  // split a single packet with multiple lines (single char)
  {
    const char *str = "hello\nworld";
    ps_t *source = create_single_value(str, strlen(str));
    REQUIRE(source != nullptr);

    ps_t *splitter = create_splitter(1, (const uint8_t *)"\n");
    REQUIRE(splitter != nullptr);

    splitter->source = source;

    // read "hello"
    {
      ps_val_t *val = splitter->fn(splitter, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(splitter->status == PS_OK);
      CHECK(val->len == 5);
      CHECK(strcmp((char *)val->data, "hello") == 0);
      ps_destroy(val);
    }

    // read "world"
    {
      ps_val_t *val = splitter->fn(splitter, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(splitter->status == PS_OK);
      CHECK(val->len == 5);
      CHECK(strcmp((char *)val->data, "world") == 0);
      ps_destroy(val);
    }

    // read NULL and ensure DONE
    {
      ps_val_t* val = splitter->fn(splitter, PS_OK);
      REQUIRE(val == nullptr);
      CHECK(splitter->status == PS_DONE);
    }

    ps_destroy(source);
    ps_destroy(splitter);
  }

  // split a single packet with multiple lines (multiple chars)
  {
    const char* str = "hello\r\n\r\nworld";
    ps_t* source = create_single_value(str, strlen(str));
    REQUIRE(source != nullptr);

    ps_t* splitter = create_splitter(4, (const uint8_t*)"\r\n\r\n");
    REQUIRE(splitter != nullptr);

    splitter->source = source;

    // read "hello"
    {
      ps_val_t* val = splitter->fn(splitter, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(splitter->status == PS_OK);
      CHECK(val->len == 5);
      CHECK(strcmp((char*)val->data, "hello") == 0);
      ps_destroy(val);
    }

    // read "world"
    {
      ps_val_t* val = splitter->fn(splitter, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(splitter->status == PS_OK);
      CHECK(val->len == 5);
      CHECK(strcmp((char*)val->data, "world") == 0);
      ps_destroy(val);
    }

    // read NULL and ensure DONE
    {
      ps_val_t* val = splitter->fn(splitter, PS_OK);
      REQUIRE(val == nullptr);
      CHECK(splitter->status == PS_DONE);
    }

    ps_destroy(source);
    ps_destroy(splitter);
  }


}