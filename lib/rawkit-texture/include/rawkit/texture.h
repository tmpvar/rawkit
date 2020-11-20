#pragma once

#include <rawkit/core.h>
#include <rawkit/hot.h>
#include <rawkit/image.h>
#include <vulkan/vulkan.h>
#include <rawkit/gpu.h>

// TODO: hack this in until we can make cimgui a sibling lib.
typedef void* ImTextureID;

typedef struct rawkit_texture_sampler_t {
  RAWKIT_RESOURCE_FIELDS

  VkSamplerCreateInfo create_info;

  VkSampler handle;
} rawkit_texture_sampler_t;

typedef struct rawkit_texture_options_t {
  rawkit_gpu_t *gpu;
  uint32_t width;
  uint32_t height;
  VkFormat format;
  VkDeviceSize size;
  const rawkit_image_t *source;
} rawkit_texture_options_t;


typedef struct rawkit_texture_t {
  RAWKIT_RESOURCE_FIELDS

  rawkit_gpu_t *gpu;
  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;
  VkImageLayout image_layout;
  rawkit_texture_options_t options;

  VkBuffer source_cpu_buffer;
  VkDeviceMemory source_cpu_buffer_memory;

  VkCommandBuffer command_buffer;

  // TODO: REMOVE
  const rawkit_texture_sampler_t *default_sampler;
} rawkit_texture_t;

#ifdef __cplusplus
extern "C" {
#endif

// TODO: hack this in until we can make cimgui a sibling lib.
ImTextureID rawkit_imgui_texture(rawkit_texture_t *texture, const rawkit_texture_sampler_t *sampler);

void rawkit_texture_destroy(rawkit_texture_t *texture);
bool rawkit_texture_init(rawkit_texture_t *texture, const rawkit_texture_options_t options);

rawkit_texture_t *_rawkit_texture_ex(
  rawkit_gpu_t *gpu,
  const char *from_file,
  const char *path,
  uv_loop_t *loop,
  rawkit_diskwatcher_t *watcher
);

const rawkit_texture_sampler_t *rawkit_texture_sampler_struct(rawkit_gpu_t *gpu, const VkSamplerCreateInfo *info);
const rawkit_texture_sampler_t *rawkit_texture_sampler(
  rawkit_gpu_t *gpu,
  VkFilter magFilter,
  VkFilter minFilter,
  VkSamplerMipmapMode mipmapMode,
  VkSamplerAddressMode addressModeU,
  VkSamplerAddressMode addressModeV,
  VkSamplerAddressMode addressModeW,
  float mipLodBias,
  VkBool32 anisotropyEnable,
  float maxAnisotropy,
  VkBool32 compareEnable,
  VkCompareOp compareOp,
  float minLod,
  float maxLod,
  VkBorderColor borderColor,
  VkBool32 unnormalizedCoordinates
);

#define rawkit_texture_ex(gpu, path, loop, diskwatcher) _rawkit_texture_ex(gpu, __FILE__, path, loop, diskwatcher)
#define rawkit_texture(path) _rawkit_texture_ex(rawkit_default_gpu(), __FILE__, path, uv_default_loop(), rawkit_default_diskwatcher())

#ifdef __cplusplus
}
#endif