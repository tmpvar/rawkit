#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <pull/stream.h>

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct rawkit_glsl_paths_t {
  char **entry;
  uint32_t count;
} rawkit_glsl_paths_t;

void rawkit_glsl_paths_destroy(rawkit_glsl_paths_t *paths);

typedef struct rawkit_glsl_t {
  uint32_t *data;
  uint64_t len;
  uint64_t bytes;
  bool valid;
  rawkit_glsl_paths_t included_files;
} rawkit_glsl_t;

rawkit_glsl_t *rawkit_glsl_compile(const char *name, const char *src, const rawkit_glsl_paths_t *include_dirs);
void rawkit_glsl_destroy(rawkit_glsl_t *ref);

ps_t *rawkit_glsl_compiler(const char *name, const rawkit_glsl_paths_t *includes);

#ifdef __cplusplus
  }
#endif
