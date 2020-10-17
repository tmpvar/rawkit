#include <doctest.h>

#include <rawkit/diskwatcher.h>

#include "util.h"

TEST_CASE("[rawkit/diskwatcher] default diskwatcher") {
  rawkit_diskwatcher_t *watcher = rawkit_default_diskwatcher();
  REQUIRE(watcher != nullptr);
  rawkit_diskwatcher_t *another = rawkit_default_diskwatcher();
  REQUIRE(another != nullptr);
  CHECK(watcher == another);
}

TEST_CASE("[rawkit/diskwatcher] diskwatcher lifecycle") {
  rawkit_diskwatcher_t *watcher = rawkit_diskwatcher();
  REQUIRE(watcher != nullptr);

  uv_loop_t loop;

  rawkit_diskwatcher_t *another = rawkit_diskwatcher_ex(&loop);
  REQUIRE(another != nullptr);
  CHECK(watcher != another);

  rawkit_diskwatcher_destroy(watcher);
  REQUIRE(watcher == nullptr);
  rawkit_diskwatcher_destroy(another);
  REQUIRE(another == nullptr);
}

TEST_CASE("[rawkit/diskwatcher] file versions") {
  uv_loop_t loop;
  uv_loop_init(&loop);


  // invalid watcher
  CHECK(rawkit_diskwatcher_file_version(NULL, "/path/to/nothing") == 0);

  // invalid file
  rawkit_diskwatcher_t *w = rawkit_diskwatcher_ex(&loop);
  REQUIRE(w != nullptr);

  CHECK(rawkit_diskwatcher_file_version(w, "/path/to/nothing") == 0);
  CHECK(rawkit_diskwatcher_file_version(w, "/path/to/nothing") == 0);

  // valid file
  char *full_path = create_tmpfile();
  REQUIRE(write_file(full_path, "first write") == 1);
  CHECK(rawkit_diskwatcher_file_version(w, full_path) == 1);
  uv_run(&loop, UV_RUN_NOWAIT);
  REQUIRE(write_file(full_path, "second write") == 1);

  int32_t sentinel = 1000;
  while (sentinel--) {
    if (rawkit_diskwatcher_file_version(w, full_path) > 1) {
      break;
    }
    uv_run(&loop, UV_RUN_NOWAIT);
  }

  rawkit_diskwatcher_destroy(w);
  REQUIRE(w == nullptr);
  uv_loop_close(&loop);
}