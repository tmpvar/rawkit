#pragma once

#include <pull/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PS_DUPLEX_FIELDS \
  PS_HANDLE_FIELDS \
  ps_stream_status status; \
  ps_t *source; \
  ps_t *sink;

typedef struct ps_duplex_t {
  PS_DUPLEX_FIELDS
} ps_duplex_t;

#define PS_DUPLEX_SOURCE_FIELDS \
  PS_FIELDS \
  ps_duplex_t *duplex; \
  ps_val_t *value;

typedef struct ps_duplex_source_t {
  PS_DUPLEX_SOURCE_FIELDS
} ps_duplex_source_t;

#define PS_DUPLEX_SINK_FIELDS \
  PS_FIELDS \
  ps_duplex_t *duplex;

typedef struct ps_duplex_sink_t {
  PS_DUPLEX_SINK_FIELDS
} ps_duplex_sink_t;

#ifdef __cplusplus
}
#endif
