#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct ps_cb_t ps_cb_t;

#include "sync.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  PS_ERR = -1,
  PS_OK = 0,
  PS_DONE = 1,
} ps_status;

typedef struct ps_value_t {
  void *data;
  int64_t len;
} ps_value_t;

// Utils
ps_status handle_status(ps_cb_t *cb, ps_status status);
ps_value_t *pull_through(ps_cb_t* cb, ps_status status);

typedef ps_value_t *(*ps_pull_fn)(ps_cb_t *cb, ps_status status);

#define PS_CB_FIELDS \
  ps_status status; \
  ps_pull_fn fn; \
  ps_cb_t *source;

typedef struct ps_cb_t {
  PS_CB_FIELDS
} ps_cb_t;

#ifdef __cplusplus
}
#endif