#include <doctest.h>

#include <pull/stream.h>

#include <string.h>

TEST_CASE("[pull/stream] user value source stream") {
  ps_t *user_value = create_user_value();

  {
    ps_user_value_from_str(
      (ps_user_value_t *)user_value,
      "abc123"
    );

    ps_val_t *val = ps_pull(user_value,  PS_OK);
    CHECK(user_value->status == PS_OK);
    REQUIRE(val != nullptr);
    CHECK(val->len == 6);
    CHECK(strcmp((char *)val->data, "abc123") == 0);
  }

  {
    ps_user_value_from_str(
      (ps_user_value_t *)user_value,
      "hello world"
    );

    ps_val_t *val = ps_pull(user_value,  PS_OK);
    CHECK(user_value->status == PS_OK);
    REQUIRE(val != nullptr);
    CHECK(val->len == 11);
    CHECK(strcmp((char *)val->data, "hello world") == 0);
  }

  ps_destroy(user_value);
}