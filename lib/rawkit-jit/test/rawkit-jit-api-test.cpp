#include <doctest.h>

#include <rawkit/jit.h>

#include "fixtures/util.h"

unsigned int api_called = 0;
static void callmemaybe(unsigned int value) {
  api_called = value;
}


TEST_CASE("[rawkit/jit/api] invalid file") {
  rawkit_jit_t *jit = rawkit_jit_create("/path/to/nothing");
  REQUIRE(jit == nullptr);
}

TEST_CASE("[rawkit/jit/api] noop") {
  rawkit_jit_t *jit = rawkit_jit_create(fixturePath("noop.c"));
  REQUIRE(jit != nullptr);

  CHECK(rawkit_jit_tick(jit) == true);
  CHECK(rawkit_jit_get_status(jit) == RAWKIT_JIT_STATUS_OK);

  rawkit_jit_destroy(jit);
  REQUIRE(jit == nullptr);
}

TEST_CASE("[rawkit/jit/api] call back into host") {
  rawkit_jit_t *jit = rawkit_jit_create(fixturePath("callmemaybe.cpp"));
  REQUIRE(jit != nullptr);

  rawkit_jit_add_export(jit, "callmemaybe", (void *)callmemaybe);
  rawkit_jit_tick(jit);
  CHECK(rawkit_jit_get_status(jit) == RAWKIT_JIT_STATUS_OK);

  rawkit_jit_call_setup(jit);
  CHECK(api_called == 0xFEED);

  rawkit_jit_call_loop(jit);
  CHECK(api_called == 0xF00D);

  rawkit_jit_destroy(jit);
  REQUIRE(jit == nullptr);
}
