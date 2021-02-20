#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define PS_EXPORT

typedef struct ps_t ps_t;

typedef enum ps_stream_status {
  PS_ERR = -1,
  PS_OK = 0,
  PS_DONE = 1,
} ps_stream_status;


// PS_VA_ARG_COUNT - count the number of VA_ARGS in both c and c++ mode
#ifdef __cplusplus
  #include <tuple>
  #define PS_VA_ARG_COUNT(...) (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)
#else
  #define PS_VA_ARG_COUNT(...) ((int)(sizeof((int[]){ __VA_ARGS__ })/sizeof(int)))
#endif


#ifdef __cplusplus
  extern "C" {
#endif

typedef struct ps_handle_t ps_handle_t;

typedef void (*ps_destroy_fn)(ps_handle_t *s);

typedef enum {
  PS_HANDLE_NONE = 0,
  PS_HANDLE_DUPLEX = 1,
  PS_HANDLE_STREAM = 2,
  PS_HANDLE_VALUE = 3,
} ps_handle_type;

#define PS_HANDLE_FIELDS \
  ps_handle_type handle_type; \
  ps_destroy_fn handle_destroy_fn;

typedef struct ps_handle_t {
  PS_HANDLE_FIELDS
} ps_handle_t;

#define PS_VALUE_FIELDS \
  PS_HANDLE_FIELDS \
  void *data; \
  int64_t len;

typedef struct ps_val_t {
  PS_VALUE_FIELDS
} ps_val_t;

typedef ps_val_t *(*ps_pull_fn)(ps_t *s, ps_stream_status status);

// Utils
PS_EXPORT ps_stream_status _ps_status(ps_handle_t *s, ps_stream_status status);
#define ps_status(handle, status) _ps_status((ps_handle_t *)handle, status)

// Service a pull stream
// - receive data from a through/source stream
// - service a sink, which in turn retrieves data from its source
PS_EXPORT ps_val_t *ps_pull(ps_t *s, ps_stream_status status);

// used internally in through-ish streams
PS_EXPORT ps_val_t *ps__pull_from_source(ps_t* s, ps_stream_status status);

PS_EXPORT void _ps_destroy(ps_handle_t **s);
#define ps_destroy(h) _ps_destroy((ps_handle_t **)&h)

PS_EXPORT ps_handle_t *_ps_create(uint64_t size, ps_handle_type type, ps_destroy_fn destroy_fn);

#define ps_create_duplex(type, destroy_fn) (type *)_ps_create(sizeof(type), PS_HANDLE_DUPLEX, destroy_fn)
#define ps_create_stream(type, destroy_fn) (type *)_ps_create(sizeof(type), PS_HANDLE_STREAM, destroy_fn)
#define ps_create_value(type, destroy_fn) (type *)_ps_create(sizeof(type), PS_HANDLE_VALUE, destroy_fn)

PS_EXPORT ps_t *_ps_pipeline(uint64_t argc, ...);
#define ps_pipeline(...) _ps_pipeline(PS_VA_ARG_COUNT(__VA_ARGS__),## __VA_ARGS__)

#define PS_FIELDS \
  PS_HANDLE_FIELDS \
  ps_stream_status status; \
  ps_pull_fn fn; \
  ps_t *source;

typedef struct ps_t {
  PS_FIELDS
} ps_t;

#include "duplex.h"
#include "io.h"
#include "sync.h"

#ifdef __cplusplus
}
#endif