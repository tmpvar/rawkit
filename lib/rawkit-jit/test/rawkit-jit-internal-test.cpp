#include <doctest.h>

#include <rawkit-jit-internal.h>

#include "fixtures/util.h"

TEST_CASE("[rawkit/jit/internal] construction") {
  {
    const char *args[] = { fixturePath("noop.c") };
    JitJob *job = JitJob::create(1, args);
    job->rebuild();
    REQUIRE(job != nullptr);
    REQUIRE(job->active_runnable);
  }

  {
    const char *args[] = { fixturePath("noop.cpp") };
    JitJob *job = JitJob::create(1, args);
    job->rebuild();
    REQUIRE(job != nullptr);
    REQUIRE(job->active_runnable);
  }
}