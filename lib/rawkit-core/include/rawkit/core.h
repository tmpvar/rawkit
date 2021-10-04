#pragma once
#include <stdint.h>
#include <stddef.h>

#define RAWKIT_DEFAULT(_value, _default) (!_value ? _default : _value)

// RAWKIT_RESOURCE_ARG_COUNT - count the number of VA_ARGS in both c and c++ mode
#ifdef __cplusplus
  #include <tuple>
  #define RAWKIT_RESOURCE_ARG_COUNT(...) (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)
#else
  #define RAWKIT_RESOURCE_ARG_COUNT(...) ((int)(sizeof((rawkit_shader_param_t[]){ __VA_ARGS__ })/sizeof(rawkit_shader_param_t)))
#endif

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct rawkit_resource_ref_t {
  uint64_t id;
  uint32_t version;
} rawkit_resource_ref_t;

#define RAWKIT_RESOURCE_FIELDS \
  const char *resource_name; \
  uint64_t resource_id; \
  rawkit_resource_ref_t *resource_source_refs; \
  uint32_t resource_version; \
  uint32_t resource_flags;

typedef struct rawkit_resource_t {
  RAWKIT_RESOURCE_FIELDS
} rawkit_resource_t;

bool _rawkit_resource_sources(rawkit_resource_t *res, uint32_t source_count, ...);
bool rawkit_resource_sources_array(rawkit_resource_t *res, uint32_t source_count, rawkit_resource_t **sources);

bool rawkit_is_debugger_attached();

#define rawkit_resource_ptr_list rawkit_resource_t **
#define rawkit_resource_sources(resource, ...) \
  _rawkit_resource_sources( \
    (rawkit_resource_t *)resource, \
    RAWKIT_RESOURCE_ARG_COUNT(__VA_ARGS__),## \
    __VA_ARGS__ \
  )


typedef int8_t    i8;
typedef uint8_t   u8;
typedef int16_t   i16;
typedef uint16_t  u16;
typedef int32_t   i32;
typedef uint32_t  u32;
typedef int64_t   i64;
typedef uint64_t  u64;
typedef float     f32;
typedef double    f64;

#ifdef RAWKIT_GUEST
typedef __int128_t i128;
typedef __uint128_t u128;
#endif

#ifndef __cplusplus
  #define min(a, b) a<b?a:b
  #define max(a, b) a>b?a:b
#endif


double rawkit_now();
float rawkit_randf();


// Args

// positional
size_t rawkit_arg_pos_count();
bool rawkit_arg_pos_bool(size_t key, bool default_value);
i8 rawkit_arg_pos_i8(size_t key, i8 default_value);
u8 rawkit_arg_pos_u8(size_t key, u8 default_value);
i16 rawkit_arg_pos_i16(size_t key, i16 default_value);
u16 rawkit_arg_pos_u16(size_t key, u16 default_value);
i32 rawkit_arg_pos_i32(size_t key, i32 default_value);
u32 rawkit_arg_pos_u32(size_t key, u32 default_value);
i64 rawkit_arg_pos_i64(size_t key, i64 default_value);
u64 rawkit_arg_pos_u64(size_t key, u64 default_value);
f32 rawkit_arg_pos_f32(size_t key, f32 default_value);
f64 rawkit_arg_pos_f64(size_t key, f64 default_value);
u32 rawkit_arg_pos_string(size_t key, char *value, size_t max_len);

// by name (e.g., `--file` is accessed via key `"file"`)
bool rawkit_arg_bool(const char *key, bool default_value);
i8 rawkit_arg_i8(const char *key, i8 default_value);
u8 rawkit_arg_u8(const char *key, u8 default_value);
i16 rawkit_arg_i16(const char *key, i16 default_value);
u16 rawkit_arg_u16(const char *key, u16 default_value);
i32 rawkit_arg_i32(const char *key, i32 default_value);
u32 rawkit_arg_u32(const char *key, u32 default_value);
i64 rawkit_arg_i64(const char *key, i64 default_value);
u64 rawkit_arg_u64(const char *key, u64 default_value);
f32 rawkit_arg_f32(const char *key, f32 default_value);
f64 rawkit_arg_f64(const char *key, f64 default_value);
u32 rawkit_arg_string(const char *key, char *value, size_t max_len);

#ifdef __cplusplus
  }
#endif


