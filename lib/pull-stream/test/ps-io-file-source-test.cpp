#include <doctest.h>

#include <pull/stream.h>

#include <stdio.h>
#include <string.h>

TEST_CASE("[pull/stream/io] file source stream") {
  // return NULL on invalid args
  {
    REQUIRE(create_file_source(NULL, NULL) == nullptr);
    uv_loop_t loop;
    REQUIRE(create_file_source(NULL, &loop) == nullptr);
    REQUIRE(create_file_source("/path/to/thing.txt", NULL) == nullptr);
  }

  // read the current file
  {
    uv_loop_t loop;
    uv_loop_init(&loop);
    ps_t *source = create_file_source(__FILE__, &loop);
    ps_t *collector = create_collector();
    collector->source = source;

    ps_val_t *val = NULL;
    while (!val && collector->status == PS_OK) {
      uv_run(&loop, UV_RUN_NOWAIT);
      val = collector->fn(collector, PS_OK);
    }

    REQUIRE(val != nullptr);
    CHECK(strstr((char *)val->data, "FIND THIS STRING") != nullptr);

    ps_destroy(val);
    ps_destroy(source);
    ps_destroy(collector);
    uv_loop_close(&loop);
  }
}
