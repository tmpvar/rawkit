#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct ps_t ps_t;

typedef enum {
  PS_ERR = -1,
  PS_OK = 0,
  PS_DONE = 1,
} ps_status;

#include "sync.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ps_value_destroy_fn)(void *s);

typedef struct ps_val_t {
  void *data;
  int64_t len;
  ps_value_destroy_fn destroy_fn;
} ps_val_t;

typedef ps_val_t *(*ps_pull_fn)(ps_t *s, ps_status status);
typedef void (*ps_destroy_fn)(ps_t *s);

// Utils
ps_status handle_status(ps_t *s, ps_status status);
ps_val_t *pull_through(ps_t* s, ps_status status);

void ps_val_destroy(ps_val_t *val);
void ps_destroy(ps_t *s);

#define PS_FIELDS \
  ps_status status; \
  ps_pull_fn fn; \
  ps_destroy_fn destroy_fn; \
  ps_t *source;

typedef struct ps_t {
  PS_FIELDS
} ps_t;

#ifdef __cplusplus
}
#endif