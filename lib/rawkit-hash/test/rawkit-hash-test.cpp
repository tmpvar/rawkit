#include <doctest.h>

#include <rawkit/hash.h>
#include <string.h>

TEST_CASE("[rawkit/hash] single hash") {
  // invalid args
  {
    REQUIRE(rawkit_hash(0, NULL) == 0);
    REQUIRE(rawkit_hash(1, NULL) == 0);
    const char *str = "hash me";
    REQUIRE(rawkit_hash(0, (void *)str) == 0);
  }

  // char * hash
  {
    const char *str = "hash me";
    const uint64_t expect_hash = 6826256565564913957;
    REQUIRE(rawkit_hash(strlen(str), (void *)str) == expect_hash);

    ps_t *hasher = ps_rawkit_hasher();

    ps_t *s = ps_pipeline(
      create_single_value((const void *)str, strlen(str)),
      hasher
    );

    REQUIRE(s != nullptr);

    {
      ps_val_t *val = s->fn(s, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(s->status == PS_OK);
      CHECK(strcmp((char *)val->data, str) == 0);
      CHECK(ps_rawkit_get_hash64(hasher) == 0);
      ps_destroy(val);
    }

    {
      ps_val_t *val = s->fn(s, PS_OK);
      REQUIRE(val == nullptr);
      CHECK(s->status == PS_DONE);
      CHECK(ps_rawkit_get_hash64(hasher) == expect_hash);
    }
  }
}

TEST_CASE("[rawkit/hash] multi-chunk") {
  const uint16_t data_len = 613;
  uint8_t data[data_len] = {0};
  for (uint32_t i=0; i<data_len; i++) {
    data[i] = i&255;
  }

  const uint64_t expect_hash = 4480806594077091881;

  REQUIRE(rawkit_hash(data_len, (void *)data) == expect_hash);

  ps_t *hasher = ps_rawkit_hasher();

  ps_t *s = ps_pipeline(
    create_single_value((void *)data, data_len),
    hasher
  );

  REQUIRE(s != nullptr);

  {
    ps_val_t *val = s->fn(s, PS_OK);
    REQUIRE(val != nullptr);
    CHECK(s->status == PS_OK);
    CHECK(memcmp(val->data, data, data_len) == 0);
    CHECK(ps_rawkit_get_hash64(hasher) == 0);
    ps_destroy(val);
  }

  {
    ps_val_t *val = s->fn(s, PS_OK);
    REQUIRE(val == nullptr);
    CHECK(s->status == PS_DONE);
    CHECK(ps_rawkit_get_hash64(hasher) == expect_hash);
  }
}