#pragma once

#include <stdint.h>

#include <vulkan/vulkan.h>
#include <uv.h>
#include <pull/stream.h>
#include <rawkit/glsl.h>

#include <rawkit/texture.h>

typedef struct rawkit_shader_uniform_buffer_t {
  rawkit_glsl_reflection_entry_t *entry;
  VkBuffer handle;
  VkDeviceMemory memory;
  uint64_t size;
  uint32_t set;
} rawkit_shader_uniform_buffer_t;

typedef struct rawkit_shader_t {
  VkDescriptorSetLayout *descriptor_set_layouts;
  uint32_t descriptor_set_layout_count;
  VkDescriptorSet *descriptor_sets;
  uint32_t descriptor_set_count;

  VkCommandPool command_pool;

  VkCommandBuffer command_buffer;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;

  VkPhysicalDevice physical_device;

  rawkit_glsl_t *glsl;

  // for graphics pipelines
  VkRenderPass render_pass;

  // managed by stb_sb internally
  VkShaderModule *modules;
  rawkit_shader_uniform_buffer_t *ubos;
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

#define rawkit_shader_texture(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_TEXTURE_PTR, .ptr = _value, .bytes = sizeof(*_value), }
#define rawkit_shader_array(_name, _len, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_ARRAY, .ptr = _value, .bytes = _len * sizeof(*_value), }
#define rawkit_shader_pull_stream(_name, _ps) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_PULL_STREAM, .ptr = _ps, .bytes = sizeof(_ps), }

#define rawkit_shader_ubo(_name, _value) (rawkit_shader_param_t){.name = _name, .type = RAWKIT_SHADER_PARAM_UNIFORM_BUFFER, .ptr = _value, .bytes = sizeof(*_value) }

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
  VkResult rawkit_shader_init(
    rawkit_glsl_t *glsl,
    rawkit_shader_t *shader
  );

  void rawkit_shader_apply_params(rawkit_shader_t *shader, VkCommandBuffer command_buffer, rawkit_shader_params_t params);
  void rawkit_shader_set_param(rawkit_shader_t *shader, const rawkit_shader_param_t *param);
  void rawkit_shader_update_ubo(rawkit_shader_t *shader, const char *name, uint32_t len, void *value);

  rawkit_shader_t *_rawkit_shader_ex(
    const char *from_file,
    uv_loop_t *loop,
    rawkit_diskwatcher_t *watcher,

    VkPhysicalDevice physical_device,
    uint32_t shader_copies,
    uint8_t source_count,
    const char **source_files
  );

  #define rawkit_shader_ex(physical_device, shader_copies, source_count, source_files, loop, diskwatcher) _rawkit_shader_ex(__FILE__, loop, diskwatcher, physical_device, shader_copies, source_count, source_files)
  #define rawkit_shader(physical_device, shader_copies, source_count, source_files) _rawkit_shader_ex(__FILE__, uv_default_loop(), rawkit_default_diskwatcher(), physical_device, shader_copies, source_count, source_files)

#ifdef __cplusplus
}
#endif
