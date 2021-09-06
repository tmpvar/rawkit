#pragma once
#include <stdint.h>

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

#ifndef __cplusplus
  #define min(a, b) a<b?a:b
  #define max(a, b) a>b?a:b
#endif


double rawkit_now();


#ifdef __cplusplus
  }
#endif


