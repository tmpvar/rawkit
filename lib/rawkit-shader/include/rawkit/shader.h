#pragma once

#include <stdint.h>

#include <vulkan/vulkan.h>
#include <pull/stream.h>
#include <rawkit/glsl.h>

#include <rawkit/texture.h>

typedef struct rawkit_shader_t {
  VkDescriptorSetLayout *descriptor_set_layouts;
  uint32_t descriptor_set_layout_count;
  VkDescriptorSet *descriptor_sets;
  uint32_t descriptor_set_count;

  VkCommandPool command_pool;
  VkShaderModule shader_module;
  VkCommandBuffer command_buffer;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
} rawkit_shader_t;


// RAWKIT_SHADER_ARG_COUNT - count the number of VA_ARGS in both c and c++ mode
#ifdef __cplusplus
  #include <tuple>
  #define RAWKIT_SHADER_ARG_COUNT(...) (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)
#else
  #define RAWKIT_SHADER_ARG_COUNT(...) ((int)(sizeof((rawkit_shader_param_t[]){ __VA_ARGS__ })/sizeof(rawkit_shader_param_t)))
#endif

#define rawkit_shader_push_constants(opts, ...) { \
  opts.count = RAWKIT_SHADER_ARG_COUNT(__VA_ARGS__); \
  opts.entries = (rawkit_shader_param_t[]){ \
    __VA_ARGS__ \
  }; \
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
} rawkit_shader_param_types_t;

#define rawkit_shader_f32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_F32, .f32 = _value, .bytes = 4, }
#define rawkit_shader_i32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_I32, .i32 = _value, .bytes = 4, }
#define rawkit_shader_u32(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_U32, .u32 = _value, .bytes = 4, }

#define rawkit_shader_f64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_F64, .f64 = _value, .bytes = 8, }
#define rawkit_shader_i64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_I64, .i64 = _value, .bytes = 8, }
#define rawkit_shader_u64(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_U64, .u64 = _value, .bytes = 8, }

#define rawkit_shader_array(_name, _len, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_ARRAY, .ptr = _value, .bytes = _len * sizeof(*_value), }
#define rawkit_shader_pull_stream(_name, _ps) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_PULL_STREAM, .ptr = _ps, .bytes = sizeof(_ps), }

typedef struct rawkit_shader_param_t {
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

    rawkit_texture_t *texture;
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
  uint32_t count;
  rawkit_shader_param_t *entries;
} rawkit_shader_params_t;

#ifdef __cplusplus
extern "C" {
#endif

  int rawkit_shader_param_size(const rawkit_shader_param_t *param);
  rawkit_shader_param_value_t rawkit_shader_param_value(rawkit_shader_param_t *param);

  // TODO: output should come thorugh as a param
  void rawkit_shader_init(
    rawkit_glsl_t *glsl,
    rawkit_shader_t *shader,
    const rawkit_shader_params_t *params,
    rawkit_texture_t *output
  );

#ifdef __cplusplus
}
#endif
