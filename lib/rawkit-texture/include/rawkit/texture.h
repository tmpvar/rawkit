#pragma once

#include <rawkit/vulkan.h>

#include <rawkit/hot.h>
#include <rawkit/image.h>

typedef struct rawkit_texture_options_t {
  uint32_t width;
  uint32_t height;
  VkFormat format;
  const rawkit_image_t *source;
} rawkit_texture_options_t;

// TODO: hack this in until we can make cimgui a sibling lib.
typedef void* ImTextureID;

typedef struct rawkit_texture_t {
  uint64_t id;
  ImTextureID imgui_texture;
  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;
  VkSampler sampler;
  rawkit_texture_options_t options;

  uint64_t source_version;
  VkBuffer source_cpu_buffer;
  VkDeviceMemory source_cpu_buffer_memory;

  VkCommandBuffer command_buffer;
} rawkit_texture_t;

#ifdef __cplusplus
extern "C" {
#endif

void rawkit_texture_destroy(rawkit_texture_t *texture);
bool rawkit_texture_init(rawkit_texture_t *texture, const rawkit_texture_options_t options);

rawkit_texture_t *_rawkit_texture_ex(
  const char *from_file,
  const char *path,
  uv_loop_t *loop,
  rawkit_diskwatcher_t *watcher
);

#define rawkit_texture_ex(path, loop, diskwatcher) _rawkit_texture_ex(__FILE__, path, loop, diskwatcher)
#define rawkit_texture(path) _rawkit_texture_ex(__FILE__, path, uv_default_loop(), rawkit_default_diskwatcher())

#ifdef __cplusplus
}
#endif