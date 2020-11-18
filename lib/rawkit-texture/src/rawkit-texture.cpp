#include <stdlib.h>

#include <rawkit/gpu.h>
#include <rawkit/hash.h>
#include <rawkit/image.h>
#include <rawkit/texture.h>

#include <string>
using namespace std;

void rawkit_texture_destroy(rawkit_texture_t *texture) {
  if (!texture) {
    return;
  }

  if (!texture->options.gpu) {
    return;
  }

  VkDevice device = texture->options.gpu->device;

  // TODO: ensure none of these resources are currently in use!
  if (device != VK_NULL_HANDLE) {
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
}

bool rawkit_texture_init(rawkit_texture_t *texture, const rawkit_texture_options_t options) {
  rawkit_gpu_t *gpu = options.gpu;
  VkDevice device = gpu->device;
  VkQueue queue = rawkit_vulkan_find_queue(gpu, VK_QUEUE_GRAPHICS_BIT);
  VkCommandPool command_pool = gpu->command_pool;
  VkPipelineCache pipeline_cache = gpu->pipeline_cache;

  // rebuild the images if the user requests a resize
  if (texture->options.width == options.width && texture->options.height == options.height) {
    return false;
  }

  printf("Rebuilding texture (%llu, '%s') (w: %u vs %u) (h: %u vs %u)\n",
    texture->resource_id,
    texture->resource_name,
    texture->options.width,
    options.width,
    texture->options.height,
    options.height
  );

  texture->options = options;
  if (device == VK_NULL_HANDLE) {
    printf("invalid vulkan device\n");
    return false;
  }

  // cleanup existing resources
  rawkit_texture_destroy(texture);

  // create a command buffer
  if (!texture->command_buffer) {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = command_pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    VkResult err = vkAllocateCommandBuffers(
      device,
      &info,
      &texture->command_buffer
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: failed to allocate command buffers\n");
      return texture;
    }
  }

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

    VkResult err = vkCreateImage(
      device,
      &info,
      NULL,
      &texture->image
    );

    if (err) {
      return false;
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
    alloc_info.memoryTypeIndex = rawkit_vulkan_find_memory_type(
      gpu,
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
      printf("ERROR: rawkit-texture: could not allocate image memory\n");
      return false;
    }

    VkResult bind_err = vkBindImageMemory(
      device,
      texture->image,
      texture->image_memory,
      0
    );
    if (bind_err != VK_SUCCESS) {
      printf("ERROR: rawkit-texture: could not bind image to image memory\n");
      return false;
    }
  }

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
      gpu->allocator,
      &texture->image_view
    );

    if (err) {
      printf("ERROR: rawkit-texture: could not create image view\n");
      return false;
    }
  }

  // transition the texture to be read/write
  {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult begin_result = vkBeginCommandBuffer(texture->command_buffer, &begin_info);

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
      texture->command_buffer,
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
      end_info.pCommandBuffers = &texture->command_buffer;
      VkResult end_result = vkEndCommandBuffer(texture->command_buffer);
      if (end_result != VK_SUCCESS) {
        printf("ERROR: rawkit-texture: could not end command buffer");
        return false;
      }

      VkResult submit_result = vkQueueSubmit(queue, 1, &end_info, VK_NULL_HANDLE);
      if (submit_result != VK_SUCCESS) {
        printf("ERROR: rawkit-texture: could not submit command buffer");
        return false;
      }
    }
  }
  texture->image_layout = VK_IMAGE_LAYOUT_GENERAL;

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

  texture->default_sampler = rawkit_texture_sampler_struct(gpu, &info);
  texture->options = options;

  return true;
}

rawkit_texture_t *_rawkit_texture_ex(
  rawkit_gpu_t *gpu,
  const char *from_file,
  const char *path,
  uv_loop_t *loop,
  rawkit_diskwatcher_t *watcher
) {

  if (!gpu) {
    return NULL;
  }

  string texture_id = string("file+rawkit-texture://") + path;

  const rawkit_image_t *img = _rawkit_image_ex(from_file, path, loop, watcher);
  uint64_t id = rawkit_hash_resources(texture_id.c_str(), 1, (const rawkit_resource_t **)&img);

  rawkit_texture_t* texture = rawkit_hot_resource_id(texture_id.c_str(), id, rawkit_texture_t);

  bool dirty = rawkit_resource_sources(texture, img);
  if (!dirty) {
    return texture;
  }

  VkDevice device = gpu->device;
  VkQueue queue = rawkit_vulkan_find_queue(gpu, VK_QUEUE_GRAPHICS_BIT);
  VkCommandPool command_pool = gpu->command_pool;

  rawkit_texture_options_t options = {};
  options.width = img->width;
  options.height = img->height;
  options.format = VK_FORMAT_R8G8B8A8_UNORM;

  // TODO: init doesn't do anything with the source! remove me
  options.gpu = gpu;

  vkDeviceWaitIdle(device);
  // cache miss
  // TODO: cleanup existing resources!!!!!!
  if (!rawkit_texture_init(texture, options)) {
    return texture;
  }

  if (!texture->source_cpu_buffer_memory) {
    // create the upload buffer
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = img->len;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult create_result = vkCreateBuffer(
      device,
      &buffer_info,
      NULL, // v->Allocator,
      &texture->source_cpu_buffer
    );

    if (create_result != VK_SUCCESS) {
      printf("ERROR: rawkit-texture: could not create upload buffer\n");
      return texture;
    }

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(
      device,
      texture->source_cpu_buffer,
      &req
    );

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = rawkit_vulkan_find_memory_type(
      gpu,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      req.memoryTypeBits
    );

    VkResult alloc_result = vkAllocateMemory(
      device,
      &alloc_info,
      NULL, // v->Allocator,
      &texture->source_cpu_buffer_memory
    );

    if (alloc_result != VK_SUCCESS) {
      printf("ERROR: rawkit-texture: could not allocate cpu buffer\n");
      return texture;
    }

    VkResult bind_result = vkBindBufferMemory(
      device,
      texture->source_cpu_buffer,
      texture->source_cpu_buffer_memory,
      0
    );

    if (bind_result != VK_SUCCESS) {
      printf("ERROR: rawkit-texture: could not bind cpu buffer memory\n");
      return texture;
    }
  }

  // upload the new image data
  if (dirty && texture->source_cpu_buffer_memory) {
    // copy the new data into the source_cpu memory
    {
      size_t size = static_cast<size_t>(img->len);
      void *pixels;
      VkResult map_result = vkMapMemory(
        device,
        texture->source_cpu_buffer_memory,
        0,
        size,
        0,
        &pixels
      );

      if (map_result != VK_SUCCESS) {
        printf("ERROR: rawkit-texture: could not map buffer\n");
        return texture;
      }

      memcpy(pixels, img->data, size);
      vkUnmapMemory(device, texture->source_cpu_buffer_memory);
    }

    // copy the data to the texture on the gpu
    {

      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      VkResult begin_result = vkBeginCommandBuffer(texture->command_buffer, &begin_info);
      if (begin_result != VK_SUCCESS) {
        printf("ERROR: rawkit-texture: could not begin command buffer");
        return texture;
      }

      VkImageMemoryBarrier copy_barrier[1] = {};
      copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      copy_barrier[0].image = texture->image;
      copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copy_barrier[0].subresourceRange.levelCount = 1;
      copy_barrier[0].subresourceRange.layerCount = 1;
      vkCmdPipelineBarrier(
        texture->command_buffer,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        copy_barrier
      );

      VkBufferImageCopy region = {};
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.layerCount = 1;
      region.imageExtent.width = img->width;
      region.imageExtent.height = img->height;

      region.imageExtent.depth = 1;
      vkCmdCopyBufferToImage(
        texture->command_buffer,
        texture->source_cpu_buffer,
        texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
      );

      VkImageMemoryBarrier use_barrier[1] = {};
      use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      use_barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
      use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      use_barrier[0].image = texture->image;
      use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      use_barrier[0].subresourceRange.levelCount = 1;
      use_barrier[0].subresourceRange.layerCount = 1;
      vkCmdPipelineBarrier(
        texture->command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        use_barrier
      );
    }

    // Blindly submit this command buffer, this is a supreme hack to get something
    // rendering on the screen.
    {
      VkSubmitInfo end_info = {};
      end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      end_info.commandBufferCount = 1;
      end_info.pCommandBuffers = &texture->command_buffer;
      VkResult end_result = vkEndCommandBuffer(texture->command_buffer);
      if (end_result != VK_SUCCESS) {
        printf("ERROR: rawkit-texture: could not end command buffer");
        return texture;
      }

      VkResult submit_result = vkQueueSubmit(queue, 1, &end_info, VK_NULL_HANDLE);
      if (submit_result != VK_SUCCESS) {
        printf("ERROR: rawkit-texture: could not submit command buffer");
        return texture;
      }
    }

    texture->resource_version++;
  }


  return texture;
}

const rawkit_texture_sampler_t *rawkit_texture_sampler_struct(rawkit_gpu_t *gpu, const VkSamplerCreateInfo *info) {
  if (!gpu || !info) {
    return nullptr;
  }

  uint64_t id = rawkit_hash(sizeof(VkSamplerCreateInfo), (void *)info);

  rawkit_texture_sampler_t *sampler = rawkit_hot_resource_id(
    "rawkit::texture::sampler",
    id,
    rawkit_texture_sampler_t
  );

  if (sampler->resource_version) {
    return sampler;
  }

  // create the image sampler
  {
    VkResult err = vkCreateSampler(
      gpu->device,
      info,
      gpu->allocator,
      &sampler->handle
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: TextureState::rebuild_sampler: unable to create sampler (%i)\n", err);
      return nullptr;
    }
  }

  sampler->resource_version++;
  return sampler;
}

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
) {
  VkSamplerCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.magFilter = magFilter;
  info.minFilter = minFilter;
  info.mipmapMode = mipmapMode;
  info.addressModeU = addressModeU;
  info.addressModeV = addressModeV;
  info.addressModeW = addressModeW;
  info.mipLodBias = mipLodBias;
  info.anisotropyEnable = anisotropyEnable;
  info.maxAnisotropy = maxAnisotropy;
  info.compareEnable = compareEnable;
  info.compareOp = compareOp;
  info.minLod = minLod;
  info.maxLod = maxLod;
  info.borderColor = borderColor;
  info.unnormalizedCoordinates = unnormalizedCoordinates;
  return rawkit_texture_sampler_struct(gpu, &info);
}

