#include <doctest.h>

#include <rawkit/hot.h>

typedef struct basic_state_t {
  int value;
} basic_state_t;

TEST_CASE("[rawkit/hot] state") {
  const char *key = "hello world";

  basic_state_t *s1 = rawkit_hot_state(key, basic_state_t);
  REQUIRE(s1 != nullptr);

  basic_state_t *s2 = rawkit_hot_state(key, basic_state_t);
  REQUIRE(s2 != nullptr);
  CHECK(s1 == s2);
}