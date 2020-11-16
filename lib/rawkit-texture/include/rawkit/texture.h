#pragma once

#include <rawkit/core.h>
#include <rawkit/hot.h>
#include <rawkit/image.h>
#include <vulkan/vulkan.h>
#include <rawkit/gpu.h>

// TODO: hack this in until we can make cimgui a sibling lib.
typedef void* ImTextureID;
#ifdef __cplusplus
extern "C" {
#endif

  extern ImTextureID rawkit_imgui_add_texture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);

#ifdef __cplusplus
}
#endif


typedef struct rawkit_texture_options_t {
  rawkit_gpu_t *gpu;
  uint32_t width;
  uint32_t height;
  VkFormat format;
  const rawkit_image_t *source;
} rawkit_texture_options_t;


typedef struct rawkit_texture_t {
  RAWKIT_RESOURCE_FIELDS

  ImTextureID imgui_texture;
  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;
  VkSampler sampler;
  VkImageLayout image_layout;
  rawkit_texture_options_t options;

  VkBuffer source_cpu_buffer;
  VkDeviceMemory source_cpu_buffer_memory;

  VkCommandBuffer command_buffer;
} rawkit_texture_t;

#ifdef __cplusplus
extern "C" {
#endif

// TODO: hack this in until we can make cimgui a sibling lib.
typedef void* ImTextureID;
extern ImTextureID rawkit_imgui_add_texture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);


void rawkit_texture_destroy(rawkit_texture_t *texture);
bool rawkit_texture_init(rawkit_texture_t *texture, const rawkit_texture_options_t options);

rawkit_texture_t *_rawkit_texture_ex(
  rawkit_gpu_t *gpu,
  const char *from_file,
  const char *path,
  uv_loop_t *loop,
  rawkit_diskwatcher_t *watcher
);

#define rawkit_texture_ex(gpu, path, loop, diskwatcher) _rawkit_texture_ex(gpu, __FILE__, path, loop, diskwatcher)
#define rawkit_texture(path) _rawkit_texture_ex(rawkit_default_gpu(), __FILE__, path, uv_default_loop(), rawkit_default_diskwatcher())

#ifdef __cplusplus
}
#endif