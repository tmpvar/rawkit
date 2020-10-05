#pragma once

#include <pull/stream.h>

#include <uv.h>

#include "duplex.h"

#ifdef __cplusplus
extern "C" {
#endif

// Filesystem sinks and sources
ps_t *create_file_source(const char *path, uv_loop_t *loop);
ps_t *create_file_sink(const char *path, uv_loop_t *loop);

// Duplex IO (TCP, UDP, IPC, etc..)
#define PS_DUPLEX_IO_FIELDS \
  PS_DUPLEX_FIELDS \
  uv_loop_t *loop; \
  uv_stream_t *stream;

typedef struct ps_duplex_io_t {
  PS_DUPLEX_IO_FIELDS
} ps_duplex_io_t;

#define PS_DUPLEX_IO_SINK_FIELDS \
  PS_DUPLEX_FIELDS \
  uv_write_t *write_req;

typedef struct ps_duplex_source_t ps_duplex_io_source_t;
typedef struct ps_duplex_io_sink_t {
  PS_DUPLEX_IO_SINK_FIELDS
} ps_duplex_io_sink_t;

ps_duplex_t *create_tcp_client(const char *addr, uint16_t port, uv_loop_t *loop);


#ifdef __cplusplus
}
#endif
