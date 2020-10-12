#include <doctest.h>

#include <rawkit-jit-internal.h>

#include "fixtures/util.h"

#include <string>
#include <unordered_map>
#include <iostream>
using namespace std;


uint32_t internal_called = 0;
static void callmemaybe(uint32_t value) {
  internal_called = value;
}

TEST_CASE("[rawkit/jit/internal] call host function") {
  const char* args[] = { fixturePath("callmemaybe.cpp") };
  JitJob* job = JitJob::create(1, args);

  job->addExport("callmemaybe", callmemaybe);
  job->rebuild();
  REQUIRE(job != nullptr);
  REQUIRE(job->active_runnable);

  job->setup();
  CHECK(internal_called == 0xFEED);

  job->loop();
  CHECK(internal_called == 0xF00D);
}