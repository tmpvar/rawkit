#pragma once

#include <vulkan/vulkan.h>

typedef struct rawkit_texture_options_t {
  uint32_t width;
  uint32_t height;
  VkFormat format;
} rawkit_texture_options_t;

typedef struct rawkit_texture_t {
  uint64_t id;
  ImTextureID imgui_texture;
  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;
  VkSampler sampler;
  rawkit_texture_options_t options;
} rawkit_texture_t;

static uint32_t find_memory_type(VkMemoryPropertyFlags properties, uint32_t type_bits) {
    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(rawkit_vulkan_physical_device(), &prop);
    for (uint32_t i = 0; i < prop.memoryTypeCount; i++) {
      if (
        (prop.memoryTypes[i].propertyFlags & properties) == properties &&
        type_bits & (1 << i)
      ) {
        return i;
      }
    }
    return 0xFFFFFFFF; // Unable to find memoryType
}

void rawkit_texture_destroy(rawkit_texture_t *texture) {
  VkDevice device = rawkit_vulkan_device();

  // TODO: ensure none of these resources are currently in use!
  if (device != VK_NULL_HANDLE) {
    if (texture->sampler != VK_NULL_HANDLE) {
      vkDestroySampler(device, texture->sampler, NULL);
    }

    if (texture->image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(device, texture->image_view, NULL);
    }

    if (texture->image != VK_NULL_HANDLE) {
      vkDestroyImage(device, texture->image, NULL);
    }

    if (texture->image_memory != VK_NULL_HANDLE) {
      vkFreeMemory(device, texture->image_memory, NULL);
    }
  }

  // TODO: free from hot state

}

void rawkit_texture_init(rawkit_texture_t *texture, const rawkit_texture_options_t options) {
  VkDevice device = rawkit_vulkan_device();
  VkQueue queue = rawkit_vulkan_queue();
  VkPipelineCache pipeline_cache = rawkit_vulkan_pipeline_cache();

  // rebuild the images if the user requests a resize
  if (texture->options.width != options.width || texture->options.height != options.height) {
    printf("Rebuilding texture\n");
    texture->options = options;
    if (device == VK_NULL_HANDLE) {
      printf("invalid vulkan device\n");
      return;
    }

    // cleanup existing resources
    rawkit_texture_destroy(texture);


    // create the image
    {
      VkImageCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      info.imageType = VK_IMAGE_TYPE_2D;
      // TODO: allow this to be provided
      info.format = options.format;
      info.extent.width = options.width;
      info.extent.height = options.height;
      info.extent.depth = 1;
      info.mipLevels = 1;
      info.arrayLayers = 1;
      info.samples = VK_SAMPLE_COUNT_1_BIT;
      info.tiling = VK_IMAGE_TILING_OPTIMAL;
      info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      VkResult result = vkCreateImage(
        device,
        &info,
        NULL,
        &texture->image
      );

      if (result != VK_SUCCESS) {
        return;
      }

      VkMemoryRequirements req;
      vkGetImageMemoryRequirements(
        device,
        texture->image,
        &req
      );

      VkMemoryAllocateInfo alloc_info = {};
      alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      alloc_info.allocationSize = req.size;
      alloc_info.memoryTypeIndex = find_memory_type(
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        req.memoryTypeBits
      );

      VkResult alloc_err = vkAllocateMemory(
        device,
        &alloc_info,
        NULL, //v->Allocator,
        &texture->image_memory
      );

      if (alloc_err != VK_SUCCESS) {
        printf("ERROR: SharedTexture: could not allocate image memory\n");
        return;
      }

      VkResult bind_err = vkBindImageMemory(
        device,
        texture->image,
        texture->image_memory,
        0
      );
      if (bind_err != VK_SUCCESS) {
        printf("ERROR: SharedTexture: could not bind image to image memory\n");
        return;
      }
    }
    printf("image created\n");
    // create the image view
    {
      VkImageViewCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      info.image = texture->image;
      info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      info.format = options.format;
      info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      info.subresourceRange.levelCount = 1;
      info.subresourceRange.layerCount = 1;
      VkResult err = vkCreateImageView(
        device,
        &info,
        NULL, // v->Allocator
        &texture->image_view
      );

      if (err != VK_SUCCESS) {
        printf("ERROR: SharedTexture: could not create image view\n");
        return;
      }
    }
    printf("image view created\n");

    // create the image sampler
    {
      VkSamplerCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
      info.magFilter = VK_FILTER_LINEAR;
      info.minFilter = VK_FILTER_LINEAR;
      info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      info.minLod = -1000;
      info.maxLod = 1000;
      info.maxAnisotropy = 1.0f;
      VkResult create_result = vkCreateSampler(
        device,
        &info,
        NULL, // v->Allocator,
        &texture->sampler
      );
      if (create_result != VK_SUCCESS) {
        printf("ERROR: SharedTexture: unable to create sampler\n");
        return;
      }
    }

    // transition the texture to be read/write
    VkCommandBuffer command_buffer = rawkit_vulkan_command_buffer();
    VkQueue queue = rawkit_vulkan_queue();
    {
      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      VkResult begin_result = vkBeginCommandBuffer(command_buffer, &begin_info);

      VkImageMemoryBarrier barrier = {};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = texture->image;
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.levelCount = 1;
      barrier.subresourceRange.layerCount = 1;
      vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrier
      );

      // Blindly submit this command buffer, this is a supreme hack to get something
      // rendering on the screen.
      {
        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        VkResult end_result = vkEndCommandBuffer(command_buffer);
        if (end_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not end command buffer");
          return;
        }

        VkResult submit_result = vkQueueSubmit(queue, 1, &end_info, VK_NULL_HANDLE);
        if (submit_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not submit command buffer");
          return;
        }

        // TODO: just ugh. this defeats the entire purpose of using vulkan.
        VkResult wait_result = vkDeviceWaitIdle(device);
        if (wait_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not wait for device idle");
          return;
        }
      }
    }

    texture->imgui_texture = rawkit_imgui_add_texture(
      texture->sampler,
      texture->image_view,
      VK_IMAGE_LAYOUT_GENERAL
    );
  }
}


rawkit_texture_t *rawkit_texture_hot(const char *name, const rawkit_texture_options_t options) {
  char id[4096] = "rawkit/texture::";
  strcat(id, name);
  rawkit_texture_t *texture = rawkit_hot_state(id, rawkit_texture_t);
  if (!texture) {
    return NULL;
  }

  rawkit_texture_init(texture, options);

  return texture;
}