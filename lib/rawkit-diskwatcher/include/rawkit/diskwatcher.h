#pragma once

#include <uv.h>

#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct rawkit_diskwatcher_t rawkit_diskwatcher_t;

rawkit_diskwatcher_t *rawkit_default_diskwatcher();

void _rawkit_diskwatcher_destroy(rawkit_diskwatcher_t **watcher);
#define rawkit_diskwatcher_destroy(watcher) _rawkit_diskwatcher_destroy(&watcher)

rawkit_diskwatcher_t *_rawkit_diskwatcher_ex(uv_loop_t *loop);

#define rawkit_diskwatcher() _rawkit_diskwatcher_ex(uv_default_loop())
#define rawkit_diskwatcher_ex(loop) _rawkit_diskwatcher_ex(loop)


uint64_t rawkit_diskwatcher_file_version(rawkit_diskwatcher_t *watcher, const char *full_path);

#ifdef __cplusplus
  }
#endif


