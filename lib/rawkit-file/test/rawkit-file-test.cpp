#include <doctest.h>

#include <rawkit/file.h>
#include "util.h"
#include <string.h>

TEST_CASE("[rawkit/file] non-existent") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    const rawkit_file_t *f = rawkit_file_ex("/path/to/nowhere.txt", &loop, NULL);
    REQUIRE(f != nullptr);
    CHECK(f->resource_version == 0);
    CHECK(f->resource_flags == 0);
  }

  uv_run(&loop, UV_RUN_DEFAULT);

  {
    const rawkit_file_t *f = rawkit_file_ex("/path/to/nowhere.txt", &loop, NULL);
    REQUIRE(f != nullptr);
    CHECK(f->resource_version == 0);
    CHECK(f->resource_flags == 0);
    CHECK(f->error == RAWKIT_FILE_NOT_FOUND);
  }
}

TEST_CASE("[rawkit/file] load existing file (absolute)") {
  const char *path = __FILE__;
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    const rawkit_file_t* f = rawkit_file_ex(path, &loop, NULL);
    REQUIRE(f != nullptr);
    CHECK(f->resource_version == 0);
    CHECK(f->resource_flags == 0);
  }

  int64_t i = 1000;
  while(i--) {
    uv_run(&loop, UV_RUN_ONCE);
    const rawkit_file_t* f = rawkit_file_ex(path, &loop, NULL);
    if (f->resource_version > 0) {
      CHECK(f->error == RAWKIT_FILE_ERROR_NONE);
      CHECK(strstr((char *)f->data, "FIND THIS STRING") != nullptr);
      break;
    }
  }

  REQUIRE(i > 0);
}

TEST_CASE("[rawkit/file] load existing file (relative)") {
  const char *path = "fixtures/static.txt";
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    const rawkit_file_t* f = rawkit_file_ex(path, &loop, NULL);
    REQUIRE(f != nullptr);
    CHECK(f->resource_version == 0);
    CHECK(f->resource_flags == 0);
  }

  int64_t i = 1000;
  while(i--) {
    uv_run(&loop, UV_RUN_ONCE);
    const rawkit_file_t* f = rawkit_file_ex(path, &loop, NULL);
    if (f->resource_version > 0) {
      CHECK(f->error == RAWKIT_FILE_ERROR_NONE);
      CHECK(strstr((char *)f->data, "this file contains a string") != nullptr);
      break;
    }
  }

  REQUIRE(i > 0);
}


TEST_CASE("[rawkit/file] reload file") {
  const char *path = create_tmpfile();
  REQUIRE(write_file(path, "first write") == 1);

  uv_loop_t loop;
  uv_loop_init(&loop);
  rawkit_diskwatcher_t *watcher = rawkit_diskwatcher_ex(&loop);
  uint32_t loaded_count = 0;
  int32_t i = 100000;
  while(i--) {
    uv_run(&loop, UV_RUN_NOWAIT);
    const rawkit_file_t* f = rawkit_file_ex(path, &loop, watcher);
    if (f->resource_version > 0) {
      loaded_count++;
      CHECK(f->error == RAWKIT_FILE_ERROR_NONE);
      if (loaded_count == 1) {
        CHECK(strstr((char *)f->data, "first write") != nullptr);
        write_file(path, "one more time");
      } else if (f->resource_version == 2) {
        CHECK(strstr((char *)f->data, "one more time") != nullptr);
        break;
      }
    }
  }

  REQUIRE(i > 0);
}
