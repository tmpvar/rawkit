#pragma once

#include <uv.h>
#include <rawkit/diskwatcher.h>

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct rawkit_image_t {
  uint32_t version;
  uint8_t *data;
  uint64_t len;
  uint32_t width;
  uint32_t height;
  uint8_t channels;
} rawkit_image_t;


const rawkit_image_t *_rawkit_image_ex(const char *from_file, const char *path, uv_loop_t *loop, rawkit_diskwatcher_t *watcher);

#define rawkit_image_ex(path, loop, diskwatcher) _rawkit_image_ex(__FILE__, path, loop, diskwatcher)
#define rawkit_image(path) _rawkit_image_ex(__FILE__, path, uv_default_loop(), rawkit_default_diskwatcher())

#ifdef __cplusplus
  }
#endif