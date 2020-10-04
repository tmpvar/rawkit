#include <doctest.h>

#include <pull/stream.h>

#include <uv.h>

#include <stdio.h>
#include <string.h>

static char *create_tmpfile() {
  char *input = tmpnam(NULL);
  size_t len = strlen(input);
  char *output = (char *)calloc(len+1, 1);
  memcpy(output, input, len);
  return output;
}

static char *readfile(const char *filename) {
  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  uint64_t fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *contents = (char *)calloc(fsize + 1, 1);
  fread(contents, 1, fsize, f);
  fclose(f);
  return contents;
}

TEST_CASE("[pull/stream/io] file sink stream") {
  // fails to construct when given invalid arguments
  {
    CHECK(create_file_sink(NULL, NULL) == nullptr);
    uv_loop_t loop;
    CHECK(create_file_sink(NULL, &loop) == nullptr);
    CHECK(create_file_sink("asdf", NULL) == nullptr);
  }

  // errors when pulled and no source
  {
    uv_loop_t loop;
    uv_loop_init(&loop);
    ps_t *sink = create_file_sink(tmpnam(NULL), &loop);
    REQUIRE(sink != nullptr);
    CHECK(sink->fn(sink, PS_OK) == nullptr);
    CHECK(sink->status == PS_ERR);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
  }

  // copy this file to a temporary file
  {
    uv_loop_t loop;
    uv_loop_init(&loop);

    char *path = create_tmpfile();

    ps_t *source = create_file_source(__FILE__, &loop);
    ps_t *sink = create_file_sink(path, &loop);
    REQUIRE(source != nullptr);
    REQUIRE(sink != nullptr);

    sink->source = source;
    int sentinel = 1000;
    while (sink->status == PS_OK && sentinel --) {
      uv_run(&loop, UV_RUN_NOWAIT);
      CHECK(sink->fn(sink, PS_OK) == nullptr);
    }

    REQUIRE(sentinel > 0);
    CHECK(sink->status == PS_DONE);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);


    char *contents = readfile(path);
    REQUIRE(contents != nullptr);
    CHECK(strstr(contents, "FIND THIS STRING") != nullptr);

    free(contents);
    free(path);
  }
}
