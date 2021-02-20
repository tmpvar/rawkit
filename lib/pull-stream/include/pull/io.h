#pragma once

#include <pull/stream.h>
#include <uv.h>
#include <stdbool.h>

#include "duplex.h"


#ifdef __cplusplus
extern "C" {
#endif

// Filesystem sinks and sources
PS_EXPORT ps_t *create_file_source(const char *path, uv_loop_t *loop);
PS_EXPORT ps_t *create_file_sink(const char *path, uv_loop_t *loop);

// Duplex IO (TCP, UDP, IPC, etc..)
#define PS_DUPLEX_IO_FIELDS \
  PS_DUPLEX_FIELDS \
  uv_loop_t *loop; \
  uv_stream_t *stream; \
  bool ready;

typedef struct ps_duplex_io_t {
  PS_DUPLEX_IO_FIELDS
} ps_duplex_io_t;

#define PS_DUPLEX_IO_SINK_FIELDS \
  PS_DUPLEX_SINK_FIELDS \
  uv_write_t *write_req; \
  bool write_pending;

#define PS_DUPLEX_IO_SOURCE_FIELDS \
  PS_DUPLEX_SOURCE_FIELDS \
  bool read_pending;


typedef struct ps_duplex_io_sink_t {
  PS_DUPLEX_IO_SINK_FIELDS
} ps_duplex_io_sink_t;

typedef struct ps_duplex_io_source_t {
  PS_DUPLEX_IO_SOURCE_FIELDS
} ps_duplex_io_source_t;

typedef struct ps_io_write_req_t {
  uv_write_t req;
  uv_buf_t buf;
  ps_val_t *val;
} ps_io_write_req_t;

PS_EXPORT void ps_uv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
PS_EXPORT void ps_uv_read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
PS_EXPORT void ps_uv_write_cb(uv_write_t* req, int status);

PS_EXPORT void ps_uv_write(ps_duplex_io_t *duplex, ps_val_t *value);

PS_EXPORT ps_val_t *ps_uv_sink_fn(ps_t *sink, ps_stream_status status);
PS_EXPORT ps_val_t *ps_uv_source_fn(ps_t *source, ps_stream_status status);



PS_EXPORT ps_duplex_t *create_tcp_client(const char *addr, uint16_t port, uv_loop_t *loop);
PS_EXPORT ps_duplex_t *create_tcp_client_from_stream(uv_tcp_t *client, uv_loop_t *loop);
PS_EXPORT ps_t *create_tcp_server(const char *ip, uint16_t port, uv_loop_t *loop);

#ifdef __cplusplus
}
#endif
