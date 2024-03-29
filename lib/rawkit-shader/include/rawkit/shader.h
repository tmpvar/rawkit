#pragma once

#include <stdint.h>

#include <rawkit/core.h>
#include <rawkit/file.h>
#include <rawkit/glsl.h>
#include <rawkit/gpu.h>
#include <rawkit/texture.h>
#include <rawkit/vulkan.h>

typedef struct rawkit_shader_t {
  RAWKIT_RESOURCE_FIELDS

  // internal
  void *_state;
} rawkit_shader_t;

typedef struct rawkit_shader_options_t {
  VkPrimitiveTopology topology;
  VkPolygonMode polygonMode;
  bool noVertexInput;
  bool alphaBlend;
  bool depthTest;
  bool depthWrite;
} rawkit_shader_options_t;

static rawkit_shader_options_t rawkit_shader_default_options() {
  rawkit_shader_options_t ret = {};
  ret.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  ret.polygonMode = VK_POLYGON_MODE_FILL;
  ret.noVertexInput = false;
  ret.alphaBlend = false;
  ret.depthTest = true;
  ret.depthWrite = true;
  return ret;
}

// RAWKIT_SHADER_ARG_COUNT - count the number of VA_ARGS in both c and c++ mode
#ifdef __cplusplus
  #include <tuple>
  #define RAWKIT_SHADER_ARG_COUNT(...) (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)
  #define RAWKIT_SHADER_FILE_ARG_COUNT(...) (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)
#else
  #define RAWKIT_SHADER_ARG_COUNT(...) ((int)(sizeof((const rawkit_shader_param_t*[]){ __VA_ARGS__ })/sizeof(const rawkit_shader_param_t*)))
  #define RAWKIT_SHADER_FILE_ARG_COUNT(...) ((int)(sizeof((const rawkit_file_t *[]){ __VA_ARGS__ })/sizeof(const rawkit_file_t *)))
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

int rawkit_shader_param_size(const rawkit_shader_param_t *param);
rawkit_shader_param_value_t rawkit_shader_param_value(rawkit_shader_param_t *param);

typedef struct rawkit_shader_instance_t {
  RAWKIT_RESOURCE_FIELDS
  rawkit_shader_t *shader;
  rawkit_gpu_t *gpu;
  bool owns_command_buffer;
  bool can_launch;
  VkCommandBuffer command_buffer;

  rawkit_gpu_queue_t queue;

  // internal
  void *_state;
} rawkit_shader_instance_t;

#ifdef __cplusplus
extern "C" {
#endif

rawkit_shader_t *rawkit_shader_ex(
  rawkit_gpu_t *gpu,
  VkRenderPass render_pass,
  const rawkit_shader_options_t *options,
  uint8_t file_count,
  const rawkit_file_t **files
);

#define rawkit_shader(...) rawkit_shader_ex( \
  rawkit_default_gpu(), \
  rawkit_vulkan_renderpass(), \
  (const rawkit_shader_options_t *)0, \
  RAWKIT_SHADER_FILE_ARG_COUNT(__VA_ARGS__), \
  (const rawkit_file_t *[]){ \
    __VA_ARGS__ \
  } \
)

#define rawkit_shader_opts(options, ...) rawkit_shader_ex( \
  rawkit_default_gpu(), \
  rawkit_vulkan_renderpass(), \
  &options, \
  RAWKIT_SHADER_FILE_ARG_COUNT(__VA_ARGS__), \
  (const rawkit_file_t *[]){ \
    __VA_ARGS__ \
  } \
)

const rawkit_glsl_t *rawkit_shader_glsl(rawkit_shader_t *shader);

VkPipelineStageFlags rawkit_glsl_vulkan_stage_flags(rawkit_glsl_stage_mask_t stage);


// Shader Instances
rawkit_shader_instance_t *rawkit_shader_instance_create_ex(
  rawkit_gpu_t *gpu,
  rawkit_shader_t *shader,
  rawkit_gpu_queue_t queue,
  uint32_t frame_idx
);

#ifdef RAWKIT_WORKER
  static inline u32 rawkit_window_frame_index() {
    return 0;
  }
#endif

#define rawkit_shader_instance_create(shader) rawkit_shader_instance_create_ex( \
  rawkit_default_gpu(), \
  shader, \
  rawkit_gpu_default_queue(), \
  rawkit_window_frame_index() \
)

#define rawkit_shader_instance_create_q(shader, queue_flags) rawkit_shader_instance_create_ex( \
  rawkit_default_gpu(), \
  shader, \
  rawkit_gpu_queue(queue_flags), \
  rawkit_window_frame_index() \
)

rawkit_shader_instance_t *rawkit_shader_instance_begin_ex(
  rawkit_shader_instance_t *inst,
  VkCommandBuffer command_buffer
);

#define rawkit_shader_instance_begin(inst) rawkit_shader_instance_begin_ex(inst, rawkit_vulkan_command_buffer())

void rawkit_shader_instance_param_texture(
  rawkit_shader_instance_t *instance,
  const char *name,
  rawkit_texture_t *texture,
  const rawkit_texture_sampler_t *sampler
);

void _rawkit_shader_instance_param_ubo(
  rawkit_shader_instance_t *instance,
  const char *name,
  void *data,
  uint64_t bytes
);


#define rawkit_shader_instance_param_ubo(instance, name, value) _rawkit_shader_instance_param_ubo( \
  instance, \
  name, \
  value, \
  sizeof(*value) \
);

void rawkit_shader_instance_param_push_constants(
  rawkit_shader_instance_t *instance,
  const void *data,
  uint64_t bytes
);

void rawkit_shader_instance_param_buffer_ex(
  rawkit_shader_instance_t *instance,
  const char *name,
  rawkit_gpu_buffer_t *buffer,
  VkDeviceSize offset,
  VkDeviceSize size
);

#define rawkit_shader_instance_param_buffer(inst, name, buffer) \
  rawkit_shader_instance_param_buffer_ex(inst, name, buffer, 0, VK_WHOLE_SIZE)


void rawkit_shader_instance_param_ssbo_ex(
  rawkit_shader_instance_t *instance,
  const char *name,
  rawkit_gpu_ssbo_t *ssbo,
  VkDeviceSize offset,
  VkDeviceSize size
);

#define rawkit_shader_instance_param_ssbo(inst, name, ssbo) \
  rawkit_shader_instance_param_ssbo_ex(inst, name, ssbo, 0, VK_WHOLE_SIZE)


void rawkit_shader_instance_apply_params(
  rawkit_shader_instance_t *instance,
  rawkit_shader_params_t params
);

void rawkit_shader_instance_dispatch_compute(
  rawkit_shader_instance_t *instance,
  uint32_t width,
  uint32_t height,
  uint32_t depth
);

VkFence rawkit_shader_instance_end_ex(rawkit_shader_instance_t *instance);

#define rawkit_shader_instance_end(instance) rawkit_shader_instance_end_ex(instance)

#ifdef __cplusplus
}
#endif