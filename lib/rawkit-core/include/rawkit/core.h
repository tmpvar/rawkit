#pragma once
#include <uv.h>
#include <stdint.h>

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
  char *resource_name; \
  uint64_t resource_id; \
  rawkit_resource_ref_t *resource_source_refs; \
  uint32_t resource_version; \
  uint32_t resource_flags;

typedef struct rawkit_resource_t {
  RAWKIT_RESOURCE_FIELDS
} rawkit_resource_t;

bool _rawkit_resource_sources(rawkit_resource_t *res, uint32_t source_count, ...);
bool rawkit_resource_sources_array(rawkit_resource_t *res, uint32_t source_count, rawkit_resource_t **sources);

#define rawkit_resource_ptr_list rawkit_resource_t **
#define rawkit_resource_sources(resource, ...) \
  _rawkit_resource_sources( \
    (rawkit_resource_t *)resource, \
    RAWKIT_RESOURCE_ARG_COUNT(__VA_ARGS__), \
    __VA_ARGS__ \
  )

#ifdef __cplusplus
  }
#endif


