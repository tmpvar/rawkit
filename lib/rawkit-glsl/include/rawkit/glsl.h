#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct rawkit_glsl_t {
  uint32_t *data;
  uint64_t len;
  bool valid;
} rawkit_glsl_t;

rawkit_glsl_t *rawkit_glsl_compile(const char *name, const char *src);


#ifdef __cplusplus
  }
#endif
