#include <doctest.h>

#include <string.h>
#include <rawkit/image.h>

TEST_CASE("[rawkit/image] load missing image") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  rawkit_diskwatcher_t* watcher = rawkit_diskwatcher_ex(&loop);

  int s = 1000;
  uint32_t version = 0;
  while (s--) {
    const rawkit_image_t* img = rawkit_image_ex(
      "fixtures/enoent.png",
      &loop,
      watcher
    );

    REQUIRE(img != nullptr);
    version = img->resource_version;
    uv_run(&loop, UV_RUN_NOWAIT);
  }

  CHECK(s < 0);
  CHECK(version == 0);
}

TEST_CASE("[rawkit/image] load image") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  rawkit_diskwatcher_t* watcher = rawkit_diskwatcher_ex(&loop);

  int s = 1000;

  while (s--) {
    const rawkit_image_t* img = rawkit_image_ex(
      "fixtures/magic-checker-2x2.png",
      &loop,
      watcher
    );

    REQUIRE(img != nullptr);

    if (img->resource_version) {
      CHECK(img->len == 16);
      CHECK(img->data != nullptr);
      CHECK(img->width == 2);
      CHECK(img->height == 2);
      CHECK(img->channels == 4);

      uint8_t expect[16] = {
        255,   0, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
        255,   0, 255, 255,
      };

      CHECK(memcmp(&expect, img->data, 16) == 0);

      break;
    }

    uv_run(&loop, UV_RUN_NOWAIT);
  }

  CHECK(s > 0);
}