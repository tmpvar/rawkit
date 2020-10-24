#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <pull/stream.h>

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct rawkit_glsl_paths_t {
  char **entry;
  uint32_t count;
} rawkit_glsl_paths_t;

void rawkit_glsl_paths_destroy(rawkit_glsl_paths_t *paths);

typedef enum  {
  RAWKIT_GLSL_REFLECTION_ENTRY_NOT_FOUND = -1,
  RAWKIT_GLSL_REFLECTION_ENTRY_NONE,
  RAWKIT_GLSL_REFLECTION_ENTRY_SUBPASS_INPUT,
  RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_INPUT,
  RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_OUTPUT,
  RAWKIT_GLSL_REFLECTION_ENTRY_SAMPLED_IMAGE,
  RAWKIT_GLSL_REFLECTION_ENTRY_SEPARATE_IMAGE,
  RAWKIT_GLSL_REFLECTION_ENTRY_SEPARATE_SAMPLER,
  RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE,
  RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER,
  RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER,
  RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER,
  RAWKIT_GLSL_REFLECTION_ENTRY_ATOMIC_BUFFER,
  RAWKIT_GLSL_REFLECTION_ENTRY_ACCELERATION_STRUCTURE,
} rawkit_glsl_reflection_entry_type;

typedef enum {
  RAWKIT_GLSL_TYPE_FLOAT
} rawkit_glsl_type;

typedef enum {
  RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA32F,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA16F,
  RAWKIT_GLSL_IMAGE_FORMAT_R32F,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA8,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA8SNORM,
  RAWKIT_GLSL_IMAGE_FORMAT_RG32F,
  RAWKIT_GLSL_IMAGE_FORMAT_RG16F,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA32I,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA16I,
  RAWKIT_GLSL_IMAGE_FORMAT_R32I,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA8I,
  RAWKIT_GLSL_IMAGE_FORMAT_RG32I,
  RAWKIT_GLSL_IMAGE_FORMAT_RG16I,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA32UI,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA16UI,
  RAWKIT_GLSL_IMAGE_FORMAT_R32UI,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA8UI,
  RAWKIT_GLSL_IMAGE_FORMAT_RG32UI,
  RAWKIT_GLSL_IMAGE_FORMAT_RG16UI,
  RAWKIT_GLSL_IMAGE_FORMAT_R11FG11FB10F,
  RAWKIT_GLSL_IMAGE_FORMAT_R16F,
  RAWKIT_GLSL_IMAGE_FORMAT_RGB10A2,
  RAWKIT_GLSL_IMAGE_FORMAT_R8,
  RAWKIT_GLSL_IMAGE_FORMAT_RG8,
  RAWKIT_GLSL_IMAGE_FORMAT_R16,
  RAWKIT_GLSL_IMAGE_FORMAT_RG16,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA16,
  RAWKIT_GLSL_IMAGE_FORMAT_R16SNORM,
  RAWKIT_GLSL_IMAGE_FORMAT_RG16SNORM,
  RAWKIT_GLSL_IMAGE_FORMAT_RGBA16SNORM,
  RAWKIT_GLSL_IMAGE_FORMAT_R8SNORM,
  RAWKIT_GLSL_IMAGE_FORMAT_RG8SNORM,
  RAWKIT_GLSL_IMAGE_FORMAT_R8UI,
  RAWKIT_GLSL_IMAGE_FORMAT_RG8UI,
  RAWKIT_GLSL_IMAGE_FORMAT_R16UI,
  RAWKIT_GLSL_IMAGE_FORMAT_RGB10A2UI,
  RAWKIT_GLSL_IMAGE_FORMAT_R8I,
  RAWKIT_GLSL_IMAGE_FORMAT_RG8I,
  RAWKIT_GLSL_IMAGE_FORMAT_R16I,
  RAWKIT_GLSL_IMAGE_FORMAT_MAX = 0xFFFFFFF,
} rawkit_glsl_image_format;

typedef enum {
  RAWKIT_GLSL_DIMS_1D = 0,
  RAWKIT_GLSL_DIMS_2D = 1,
  RAWKIT_GLSL_DIMS_3D = 2,
  RAWKIT_GLSL_DIMS_CUBE = 3,
  RAWKIT_GLSL_DIMS_RECT = 4,
  RAWKIT_GLSL_DIMS_BUFFER = 5,
  RAWKIT_GLSL_DIMS_SUBPASS_DATA = 6,
  RAWKIT_GLSL_DIMS_MAX = 0x7fffffff,
} rawkit_glsl_dims;

typedef struct rawkit_glsl_reflection_entry_t {
  rawkit_glsl_reflection_entry_type entry_type;
  bool readable;
  bool writable;
  int32_t location;
  int32_t set;
  int32_t binding;
  int32_t offset;
  uint32_t block_size;
  int32_t input_attachment_index;
  rawkit_glsl_dims dims;
  rawkit_glsl_image_format image_format;
} rawkit_glsl_reflection_entry_t;


typedef struct rawkit_glsl_t rawkit_glsl_t;

rawkit_glsl_t *rawkit_glsl_compile(const char *name, const char *src, const rawkit_glsl_paths_t *include_dirs);
void rawkit_glsl_destroy(rawkit_glsl_t *ref);

bool rawkit_glsl_valid(const rawkit_glsl_t *ref);
const uint32_t *rawkit_glsl_workgroup_size(const rawkit_glsl_t *ref);
const uint32_t *rawkit_glsl_spirv_data(const rawkit_glsl_t *ref);
const uint64_t rawkit_glsl_spirv_byte_len(const rawkit_glsl_t *ref);

const rawkit_glsl_reflection_entry_t rawkit_glsl_reflection_entry(const rawkit_glsl_t* ref, const char* name);
ps_t *rawkit_glsl_compiler(const char *name, const rawkit_glsl_paths_t *includes);

#ifdef __cplusplus
  }
#endif
