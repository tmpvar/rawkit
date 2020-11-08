#pragma once
#include <uv.h>

#include <rawkit/hot.h>
#include <rawkit/diskwatcher.h>
#include <rawkit/core.h>
#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif

  typedef enum {
    RAWKIT_FILE_OOM = -2,
    RAWKIT_FILE_NOT_FOUND = -1,
    RAWKIT_FILE_ERROR_NONE = 0,
  } rawkit_file_error;

  typedef struct rawkit_file_t {
    RAWKIT_RESOURCE_FIELDS

    rawkit_file_error error;
    uint8_t *data;
    uint64_t len;

    // internal state tracking
    void *_state;
  } rawkit_file_t;

  const rawkit_file_t *_rawkit_file_ex(
    const char *from_file,
    const char *path,
    uv_loop_t *loop,
    rawkit_diskwatcher_t *watcher
  );

  #define rawkit_file_ex(path, loop, diskwatcher) _rawkit_file_ex(__FILE__, path, loop, diskwatcher)
  #define rawkit_file(path) _rawkit_file_ex(__FILE__, path, uv_default_loop(), rawkit_default_diskwatcher())

#ifdef __cplusplus
  }
#endif


