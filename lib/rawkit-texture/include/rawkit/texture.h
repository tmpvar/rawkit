#pragma once

#include <rawkit/vulkan.h>

#include <rawkit/hot.h>

typedef struct rawkit_texture_options_t {
  uint32_t width;
  uint32_t height;
  VkFormat format;
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
} rawkit_texture_t;

#ifdef __cplusplus
extern "C" {
#endif

void rawkit_texture_destroy(rawkit_texture_t *texture);
void rawkit_texture_init(rawkit_texture_t *texture, const rawkit_texture_options_t options);
rawkit_texture_t *rawkit_texture_hot(const char *name, const rawkit_texture_options_t options);

#ifdef __cplusplus
}
#endif