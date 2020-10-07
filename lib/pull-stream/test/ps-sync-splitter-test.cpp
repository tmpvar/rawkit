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


  // multiple packets with single line and a multibyte pattern
  {
    ps_t *user_value = create_user_value();
    REQUIRE(user_value != nullptr);

    ps_t *splitter = create_splitter(2, (uint8_t *)"\r\n");
    REQUIRE(splitter != nullptr);

    splitter->source = user_value;

    ps_user_value_t * uv = (ps_user_value_t *)user_value;

    ps_user_value_from_str(uv, "H");
    REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
    ps_user_value_from_str(uv, "E");
    REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
    ps_user_value_from_str(uv, "L");
    REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
    ps_user_value_from_str(uv, "L");
    REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
    ps_user_value_from_str(uv, "O");
    REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
    ps_status(user_value, PS_DONE);

    ps_val_t *val = splitter->fn(splitter, PS_OK);
    REQUIRE(val != nullptr);
    REQUIRE(val->data != nullptr);
    CHECK(val->len == 5);
    CHECK(strcmp((char *)val->data, "HELLO") == 0);

    ps_destroy(val);
    ps_destroy(splitter);
    ps_destroy(user_value);
  }

  // multiple packets with multiple lines and a multibyte pattern
  {
    ps_t *user_value = create_user_value();
    REQUIRE(user_value != nullptr);

    ps_t *splitter = create_splitter(2, (uint8_t *)"\r\n");
    REQUIRE(splitter != nullptr);

    splitter->source = user_value;

    ps_user_value_t * uv = (ps_user_value_t *)user_value;

    // first line
    {
      ps_user_value_from_str(uv, "H");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "E");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "L");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "L");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "O");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "\r\n");
      ps_val_t *val = splitter->fn(splitter, PS_OK);
      REQUIRE(val != nullptr);
      REQUIRE(val->data != nullptr);
      CHECK(val->len == 5);
      CHECK(strcmp((char *)val->data, "HELLO") == 0);
      ps_destroy(val);
    }

    // second line
    {
      ps_user_value_from_str(uv, "W");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "O");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "R");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "L");
      REQUIRE(splitter->fn(splitter, PS_OK) == nullptr);
      ps_user_value_from_str(uv, "D");
      ps_status(uv, PS_DONE);

      ps_val_t *val = splitter->fn(splitter, PS_OK);
      REQUIRE(val != nullptr);
      REQUIRE(val->data != nullptr);
      CHECK(val->len == 5);
      CHECK(strcmp((char *)val->data, "WORLD") == 0);
      ps_destroy(val);
    }

    ps_destroy(splitter);
    ps_destroy(user_value);
  }

  // TODO: test the case where the separator is at EOF
}