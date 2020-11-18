#pragma once

#include <stdint.h>

#include <rawkit/core.h>
#include <rawkit/file.h>
#include <rawkit/gpu.h>
#include <rawkit/texture.h>

typedef struct rawkit_shader_t {
  RAWKIT_RESOURCE_FIELDS

  // internal
  void *_state;
} rawkit_shader_t;


// RAWKIT_SHADER_ARG_COUNT - count the number of VA_ARGS in both c and c++ mode
#ifdef __cplusplus
  #include <tuple>
  #define RAWKIT_SHADER_ARG_COUNT(...) (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)
#else
  #define RAWKIT_SHADER_ARG_COUNT(...) ((int)(sizeof((rawkit_shader_param_t[]){ __VA_ARGS__ })/sizeof(rawkit_shader_param_t)))
#endif

#define rawkit_shader_params(opts, ...) { \
  opts.count = RAWKIT_SHADER_ARG_COUNT(__VA_ARGS__); \
  opts.entries = (rawkit_shader_param_t[]){ \
    __VA_ARGS__ \
  }; \
  rawkit_shader_params_init_resource(&opts); \
}

typedef enum {
  RAWKIT_SHADER_PARAM_F32,
  RAWKIT_SHADER_PARAM_I32,
  RAWKIT_SHADER_PARAM_U32,

  RAWKIT_SHADER_PARAM_F64,
  RAWKIT_SHADER_PARAM_I64,
  RAWKIT_SHADER_PARAM_U64,

  RAWKIT_SHADER_PARAM_PTR,
  RAWKIT_SHADER_PARAM_TEXTURE_PTR,
  RAWKIT_SHADER_PARAM_PULL_STREAM,

  RAWKIT_SHADER_PARAM_UNIFORM_BUFFER,
} rawkit_shader_param_types_t;

#define rawkit_shader_f32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_F32, .f32 = _value, .bytes = 4, }
#define rawkit_shader_i32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_I32, .i32 = _value, .bytes = 4, }
#define rawkit_shader_u32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_U32, .u32 = _value, .bytes = 4, }

#define rawkit_shader_f64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_F64, .f64 = _value, .bytes = 8, }
#define rawkit_shader_i64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_I64, .i64 = _value, .bytes = 8, }
#define rawkit_shader_u64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_U64, .u64 = _value, .bytes = 8, }

#define rawkit_shader_texture(_name, _texture, _sampler) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_TEXTURE_PTR, .texture = {.texture = _texture, .sampler = _sampler}, .bytes = sizeof(rawkit_shader_param_texture_t), }
#define rawkit_shader_array(_name, _len, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_ARRAY, .ptr = _value, .bytes = _len * sizeof(*_value), }
#define rawkit_shader_pull_stream(_name, _ps) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_PULL_STREAM, .ptr = _ps, .bytes = sizeof(_ps), }

#define rawkit_shader_ubo(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_UNIFORM_BUFFER, .ptr = _value, .bytes = sizeof(*_value) }


typedef struct rawkit_shader_param_texture_t {
  rawkit_texture_t *texture;
  const rawkit_texture_sampler_t *sampler;
} rawkit_shader_param_texture_t;

typedef struct rawkit_shader_param_t {
  RAWKIT_RESOURCE_FIELDS

  const char *name;
  uint32_t type;
  union {
    uint64_t value;
    float f32;
    float i32;
    float u32;
    double f64;
    double i64;
    double u64;

    rawkit_shader_param_texture_t texture;

    void *ptr;
    ps_t *pull_stream;

  };
  uint64_t bytes;
} rawkit_shader_param_t;

typedef struct rawkit_shader_param_value_t {
  bool should_free;
  void *buf;
  uint64_t len;
} rawkit_shader_param_value_t;

typedef struct rawkit_shader_params_t {
  RAWKIT_RESOURCE_FIELDS

  uint32_t count;
  rawkit_shader_param_t *entries;
} rawkit_shader_params_t;

#ifdef __cplusplus
extern "C" {
#endif

// TODO: we can't make this nicer to use until all of the vulkan bits migrate to rawkit-gpu.
//       I'm thinking there is a `rawkit_gpu_set_current()` or similar to control which gpu
//       `rawkit_gpu_current()` returns. However, this approach is not thread safe!

rawkit_shader_t *rawkit_shader_ex(
  rawkit_gpu_t *gpu,
  uint8_t concurrency,
  VkRenderPass render_pass,
  uint8_t file_count,
  const rawkit_file_t **files
);


void rawkit_shader_apply_params(
  rawkit_shader_t *shader,
  uint8_t concurrency_index,
  VkCommandBuffer command_buffer,
  rawkit_shader_params_t params
);
int rawkit_shader_param_size(const rawkit_shader_param_t *param);
rawkit_shader_param_value_t rawkit_shader_param_value(rawkit_shader_param_t *param);

void rawkit_shader_bind(
  rawkit_shader_t *shader,
  uint8_t concurrency_index,
  VkCommandBuffer command_buffer,
  rawkit_shader_params_t params
);

VkCommandBuffer rawkit_shader_command_buffer(rawkit_shader_t *shader, uint8_t concurrency_index);

const rawkit_glsl_t *rawkit_shader_glsl(rawkit_shader_t *shader);

void rawkit_shader_params_init_resource(rawkit_shader_params_t *params);

#ifdef __cplusplus
}
#endif