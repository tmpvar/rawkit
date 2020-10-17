#include <doctest.h>

#include <rawkit/file.h>

#include <string.h>

TEST_CASE("[rawkit/file] non-existent") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    const rawkit_file_t *f = rawkit_file_ex("/path/to/nowhere.txt", &loop);
    REQUIRE(f == nullptr);
  }

  uv_run(&loop, UV_RUN_DEFAULT);

  {
    const rawkit_file_t *f = rawkit_file_ex("/path/to/nowhere.txt", &loop);
    REQUIRE(f != nullptr);
    CHECK(f->error == RAWKIT_FILE_NOT_FOUND);
  }
}

TEST_CASE("[rawkit/file] load existing file (absolute)") {
  const char *path = __FILE__;
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    const rawkit_file_t* f = rawkit_file_ex(path, &loop);
    REQUIRE(f == nullptr);
  }

  int64_t i = 1000;
  while(i--) {
    uv_run(&loop, UV_RUN_ONCE);
    const rawkit_file_t* f = rawkit_file_ex(path, &loop);
    if (f != NULL) {
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
    const rawkit_file_t* f = rawkit_file_ex(path, &loop);
    REQUIRE(f == nullptr);
  }

  int64_t i = 1000;
  while(i--) {
    uv_run(&loop, UV_RUN_ONCE);
    const rawkit_file_t* f = rawkit_file_ex(path, &loop);
    if (f != NULL) {
      CHECK(f->error == RAWKIT_FILE_ERROR_NONE);
      CHECK(strstr((char *)f->data, "this file contains a string") != nullptr);
      break;
    }
  }

  REQUIRE(i > 0);
}
