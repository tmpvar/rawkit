#include <doctest.h>

#include <rawkit-jit-internal.h>

#include "fixtures/util.h"

#include <string>
#include <unordered_map>
#include <iostream>
using namespace std;


__m128 internal_called;
static void callmemaybe(__m128 value) {
  internal_called = value;
}

TEST_CASE("[rawkit/jit/intrinsics] call fn with the result of _mm_add_ps") {
  const char* args[] = {
    fixturePath("intrin.c")
  };
  JitJob* job = JitJob::create(1, args);

  job->addExport("callmemaybe", (void *)callmemaybe);
  job->rebuild();
  REQUIRE(job != nullptr);
  REQUIRE(job->active_runnable);

  job->setup();

  float results[4] = {0.0, 0.0, 0.0, 0.0};
  _mm_store_ps((float *)results, internal_called);
  CHECK(results[3] == 0.35f);
  CHECK(results[2] == 0.7f);
  CHECK(results[1] == 1.05f);
  CHECK(results[0] == 1.4f);
}