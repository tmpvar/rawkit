/*
The MIT License (MIT)
Copyright © 2020 Elijah Insua <tmpvar@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the “Software”), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#pragma once

#include <vulkan/vulkan.h>
#include <cimgui.h>


uint64_t computeTextureSize(uint32_t width, uint32_t height, VkFormat format);

static uint32_t find_memory_type(VkMemoryPropertyFlags properties, uint32_t type_bits)
{
    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(rawkit_vulkan_physical_device(), &prop);
    for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
        if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
            return i;
    return 0xFFFFFFFF; // Unable to find memoryType
}

enum TextureComponentType {
  TEXTURE_COMPONENT_TYPE_I8,
  TEXTURE_COMPONENT_TYPE_U8,
  TEXTURE_COMPONENT_TYPE_I16,
  TEXTURE_COMPONENT_TYPE_U16,
  TEXTURE_COMPONENT_TYPE_I32,
  TEXTURE_COMPONENT_TYPE_U32,
  TEXTURE_COMPONENT_TYPE_F32,
  TEXTURE_COMPONENT_TYPE_I64,
  TEXTURE_COMPONENT_TYPE_U64,
  TEXTURE_COMPONENT_TYPE_F64,
};

VkFormat computeTextureFormat(uint8_t channels, TextureComponentType type) {
  switch (type) {
    case TEXTURE_COMPONENT_TYPE_I8:
        switch (channels) {
          case 1: return VK_FORMAT_R8_SINT; break;
          case 2: return VK_FORMAT_R8G8_SINT; break;
          case 3: return VK_FORMAT_R8G8B8_SINT; break;
          case 4: return VK_FORMAT_R8G8B8A8_SINT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_U8:
        switch (channels) {
          case 1: return VK_FORMAT_R8_UINT; break;
          case 2: return VK_FORMAT_R8G8_UINT; break;
          case 3: return VK_FORMAT_R8G8B8_UINT; break;
          case 4: return VK_FORMAT_R8G8B8A8_UINT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_I16:
        switch (channels) {
          case 1: return VK_FORMAT_R16_SINT; break;
          case 2: return VK_FORMAT_R16G16_SINT; break;
          case 3: return VK_FORMAT_R16G16B16_SINT; break;
          case 4: return VK_FORMAT_R16G16B16A16_SINT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_U16:
        switch (channels) {
          case 1: return VK_FORMAT_R16_UINT; break;
          case 2: return VK_FORMAT_R16G16_UINT; break;
          case 3: return VK_FORMAT_R16G16B16_UINT; break;
          case 4: return VK_FORMAT_R16G16B16A16_UINT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_I32:
        switch (channels) {
          case 1: return VK_FORMAT_R32_SINT; break;
          case 2: return VK_FORMAT_R32G32_SINT; break;
          case 3: return VK_FORMAT_R32G32B32_SINT; break;
          case 4: return VK_FORMAT_R32G32B32A32_SINT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_U32:
        switch (channels) {
          case 1: return VK_FORMAT_R32_UINT; break;
          case 2: return VK_FORMAT_R32G32_UINT; break;
          case 3: return VK_FORMAT_R32G32B32_UINT; break;
          case 4: return VK_FORMAT_R32G32B32A32_UINT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_F32:
        switch (channels) {
          case 1: return VK_FORMAT_R32_SFLOAT; break;
          case 2: return VK_FORMAT_R32G32_SFLOAT; break;
          case 3: return VK_FORMAT_R32G32B32_SFLOAT; break;
          case 4: return VK_FORMAT_R32G32B32A32_SFLOAT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_I64:
        switch (channels) {
          case 1: return VK_FORMAT_R64_SINT; break;
          case 2: return VK_FORMAT_R64G64_SINT; break;
          case 3: return VK_FORMAT_R64G64B64_SINT; break;
          case 4: return VK_FORMAT_R64G64B64A64_SINT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_U64:
        switch (channels) {
          case 1: return VK_FORMAT_R64_UINT; break;
          case 2: return VK_FORMAT_R64G64_UINT; break;
          case 3: return VK_FORMAT_R64G64B64_UINT; break;
          case 4: return VK_FORMAT_R64G64B64A64_UINT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    case TEXTURE_COMPONENT_TYPE_F64:
        switch (channels) {
          case 1: return VK_FORMAT_R64_SFLOAT; break;
          case 2: return VK_FORMAT_R64G64_SFLOAT; break;
          case 3: return VK_FORMAT_R64G64B64_SFLOAT; break;
          case 4: return VK_FORMAT_R64G64B64A64_SFLOAT; break;
          default:
            return VK_FORMAT_UNDEFINED;
        }
      break;
    default:
      return VK_FORMAT_UNDEFINED;
  }

}

class SharedTexture {
  private:
    void *pixels = nullptr;
  public:
    ImTextureID imgui_texture;
    VkImage image;
    VkDeviceMemory image_memory;
    VkImageView image_view;
    VkSampler sampler;
    VkBuffer cpu_buffer;
    VkDeviceMemory cpu_buffer_memory;
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    TextureComponentType type;
    VkFormat format;
    uint64_t bytes;
    // total number of components (w * h * channels)
    uint64_t length;

    SharedTexture *init(uint32_t width, uint32_t height, uint8_t channels, TextureComponentType type) {
      // TODO: compute a hash over the args instead of this primitive check
      if (this->width == width && this->height == height) {
        return this;
      }

      this->channels = channels;
      this->type = type;

      this->format = computeTextureFormat(channels, type);

      this->width = width;
      this->height = height;
      this->length = width * height * channels;
      this->bytes = computeTextureSize(width, height, this->format);

      VkDevice device = rawkit_vulkan_device();

      // create the image
      {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = this->format;
        info.extent.width = this->width;
        info.extent.height = this->height;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult result = vkCreateImage(
          device,
          &info,
          NULL,
          &this->image
        );

        if (result != VK_SUCCESS) {
          return nullptr;
        }

        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(
          device,
          this->image,
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
          &this->image_memory
        );

        if (alloc_err != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not allocate image memory\n");
          return nullptr;
        }

        VkResult bind_err = vkBindImageMemory(
          device,
          this->image,
          this->image_memory,
          0
        );
        if (bind_err != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not bind image to image memory\n");
          return nullptr;
        }
      }

      // create the image view
      {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = this->image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = this->format;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        VkResult err = vkCreateImageView(
          device,
          &info,
          NULL, // v->Allocator
          &this->image_view
        );

        if (err != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not create image view\n");
        }
      }

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
          &this->sampler
        );
        if (create_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: unable to create sampler\n");
          return nullptr;
        }
      }

      this->imgui_texture = rawkit_imgui_add_texture(
        this->sampler,
        this->image_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      );

      // create the upload buffer
      {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = this->bytes;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkResult create_result = vkCreateBuffer(
          device,
          &buffer_info,
          NULL, // v->Allocator,
          &this->cpu_buffer
        );

        if (create_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not create upload buffer\n");
          return nullptr;
        }

        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(
          device,
          this->cpu_buffer,
          &req
        );

        // TODO: is this needed?
        // g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment)
        // ? g_BufferMemoryAlignment
        // : req.alignment;

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = find_memory_type(
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
          req.memoryTypeBits
        );

        VkResult alloc_result = vkAllocateMemory(
          device,
          &alloc_info,
          NULL, // v->Allocator,
          &this->cpu_buffer_memory
        );
        if (alloc_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not allocate cpu buffer\n");
          return nullptr;
        }

        VkResult bind_result = vkBindBufferMemory(
          device,
          this->cpu_buffer,
          this->cpu_buffer_memory,
          0
        );
        if (bind_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not bind cpu buffer memory\n");
          return nullptr;
        }
      }

      return this;
    }

    void *map() {
      if (this->pixels != nullptr) {
        return this->pixels;
      }

      VkDevice device = rawkit_vulkan_device();
      VkResult map_result = vkMapMemory(
        device,
        this->cpu_buffer_memory,
        0,
        this->bytes,
        0,
        (void**)(&this->pixels)
      );

      if (map_result != VK_SUCCESS) {
        printf("ERROR: SharedTexture: could not map buffer\n");
        return nullptr;
      }
      return this->pixels;
    }
    // TODO: would it be better to memcpy instead of holding a mapped buffer?
    SharedTexture *unmap() {
      if (this->pixels == nullptr) {
        return this;
      }

      this->pixels = nullptr;

      VkDevice device = rawkit_vulkan_device();

      // unmap the cpu buffer
      {
        VkMappedMemoryRange range[1] = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = this->cpu_buffer_memory;
        range[0].size = this->bytes;
        VkResult flush_result = vkFlushMappedMemoryRanges(
          device,
          1,
          range
        );

        if (flush_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: failed to flush writes while unmapping\n");
          return nullptr;
        }
        vkUnmapMemory(device, this->cpu_buffer_memory);
      }


      VkCommandPool command_pool = rawkit_vulkan_command_pool();
      VkCommandBuffer command_buffer = rawkit_vulkan_command_buffer();

      // prepare the command pool and command buffer to handle the upload
      {
        // TODO: is this required? It seems like this could potentially blow away
        // frame commands, so maybe this needs to be
        VkResult reset_result = vkResetCommandPool(device, command_pool, 0);
        if (reset_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not reset command pool");
          return nullptr;
        }

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VkResult begin_result = vkBeginCommandBuffer(command_buffer, &begin_info);
        if (begin_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not begin command buffer");
          return nullptr;
        }
      }

      // copy the data to the texture on the gpu
      {
        VkImageMemoryBarrier copy_barrier[1] = {};
        copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].image = this->image;
        copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_barrier[0].subresourceRange.levelCount = 1;
        copy_barrier[0].subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(
          command_buffer,
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
        region.imageExtent.width = this->width;
        region.imageExtent.height = this->height;
        // TODO: don't assume 2d!
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(
          command_buffer,
          this->cpu_buffer,
          this->image,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          1,
          &region
        );

        VkImageMemoryBarrier use_barrier[1] = {};
        use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].image = this->image;
        use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        use_barrier[0].subresourceRange.levelCount = 1;
        use_barrier[0].subresourceRange.layerCount = 1;
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
          use_barrier
        );
      }

      VkQueue queue = rawkit_vulkan_queue();
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
          return nullptr;
        }

        VkResult submit_result = vkQueueSubmit(queue, 1, &end_info, VK_NULL_HANDLE);
        if (submit_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not submit command buffer");
          return nullptr;
        }

        // TODO: just ugh. this defeats the entire purpose of using vulkan.
        VkResult wait_result = vkDeviceWaitIdle(device);
        if (wait_result != VK_SUCCESS) {
          printf("ERROR: SharedTexture: could not wait for device idle");
          return nullptr;
        }
      }

      return this;
    }

};


uint64_t computeTextureSize(uint32_t width, uint32_t height, VkFormat format) {
  uint32_t bits = 0;
  switch (format) {
    case VK_FORMAT_R4G4_UNORM_PACK8: bits = 8; break;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16: bits = 16; break;
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16: bits = 16; break;
    case VK_FORMAT_R5G6B5_UNORM_PACK16: bits = 16; break;
    case VK_FORMAT_B5G6R5_UNORM_PACK16: bits = 16; break;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16: bits = 16; break;
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16: bits = 16; break;
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16: bits = 16; break;
    case VK_FORMAT_R8_UNORM: bits = 8; break;
    case VK_FORMAT_R8_SNORM: bits = 8; break;
    case VK_FORMAT_R8_USCALED: bits = 8; break;
    case VK_FORMAT_R8_SSCALED: bits = 8; break;
    case VK_FORMAT_R8_UINT: bits = 8; break;
    case VK_FORMAT_R8_SINT: bits = 8; break;
    case VK_FORMAT_R8_SRGB: bits = 8; break;
    case VK_FORMAT_R8G8_UNORM: bits = 16; break;
    case VK_FORMAT_R8G8_SNORM: bits = 16; break;
    case VK_FORMAT_R8G8_USCALED: bits = 16; break;
    case VK_FORMAT_R8G8_SSCALED: bits = 16; break;
    case VK_FORMAT_R8G8_UINT: bits = 16; break;
    case VK_FORMAT_R8G8_SINT: bits = 16; break;
    case VK_FORMAT_R8G8_SRGB: bits = 16; break;
    case VK_FORMAT_R8G8B8_UNORM: bits = 24; break;
    case VK_FORMAT_R8G8B8_SNORM: bits = 24; break;
    case VK_FORMAT_R8G8B8_USCALED: bits = 24; break;
    case VK_FORMAT_R8G8B8_SSCALED: bits = 24; break;
    case VK_FORMAT_R8G8B8_UINT: bits = 24; break;
    case VK_FORMAT_R8G8B8_SINT: bits = 24; break;
    case VK_FORMAT_R8G8B8_SRGB: bits = 24; break;
    case VK_FORMAT_B8G8R8_UNORM: bits = 24; break;
    case VK_FORMAT_B8G8R8_SNORM: bits = 24; break;
    case VK_FORMAT_B8G8R8_USCALED: bits = 24; break;
    case VK_FORMAT_B8G8R8_SSCALED: bits = 24; break;
    case VK_FORMAT_B8G8R8_UINT: bits = 24; break;
    case VK_FORMAT_B8G8R8_SINT: bits = 24; break;
    case VK_FORMAT_B8G8R8_SRGB: bits = 24; break;
    case VK_FORMAT_R8G8B8A8_UNORM: bits = 32; break;
    case VK_FORMAT_R8G8B8A8_SNORM: bits = 32; break;
    case VK_FORMAT_R8G8B8A8_USCALED: bits = 32; break;
    case VK_FORMAT_R8G8B8A8_SSCALED: bits = 32; break;
    case VK_FORMAT_R8G8B8A8_UINT: bits = 32; break;
    case VK_FORMAT_R8G8B8A8_SINT: bits = 32; break;
    case VK_FORMAT_R8G8B8A8_SRGB: bits = 32; break;
    case VK_FORMAT_B8G8R8A8_UNORM: bits = 32; break;
    case VK_FORMAT_B8G8R8A8_SNORM: bits = 32; break;
    case VK_FORMAT_B8G8R8A8_USCALED: bits = 32; break;
    case VK_FORMAT_B8G8R8A8_SSCALED: bits = 32; break;
    case VK_FORMAT_B8G8R8A8_UINT: bits = 32; break;
    case VK_FORMAT_B8G8R8A8_SINT: bits = 32; break;
    case VK_FORMAT_B8G8R8A8_SRGB: bits = 32; break;
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32: bits = 32; break;
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32: bits = 32; break;
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32: bits = 32; break;
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: bits = 32; break;
    case VK_FORMAT_A8B8G8R8_UINT_PACK32: bits = 32; break;
    case VK_FORMAT_A8B8G8R8_SINT_PACK32: bits = 32; break;
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32: bits = 32; break;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32: bits = 32; break;
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32: bits = 32; break;
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32: bits = 32; break;
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: bits = 32; break;
    case VK_FORMAT_A2R10G10B10_UINT_PACK32: bits = 32; break;
    case VK_FORMAT_A2R10G10B10_SINT_PACK32: bits = 32; break;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: bits = 32; break;
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32: bits = 32; break;
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32: bits = 32; break;
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: bits = 32; break;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32: bits = 32; break;
    case VK_FORMAT_A2B10G10R10_SINT_PACK32: bits = 32; break;
    case VK_FORMAT_R16_UNORM: bits = 16; break;
    case VK_FORMAT_R16_SNORM: bits = 16; break;
    case VK_FORMAT_R16_USCALED: bits = 16; break;
    case VK_FORMAT_R16_SSCALED: bits = 16; break;
    case VK_FORMAT_R16_UINT: bits = 16; break;
    case VK_FORMAT_R16_SINT: bits = 16; break;
    case VK_FORMAT_R16_SFLOAT: bits = 16; break;
    case VK_FORMAT_R16G16_UNORM: bits = 32; break;
    case VK_FORMAT_R16G16_SNORM: bits = 32; break;
    case VK_FORMAT_R16G16_USCALED: bits = 32; break;
    case VK_FORMAT_R16G16_SSCALED: bits = 32; break;
    case VK_FORMAT_R16G16_UINT: bits = 32; break;
    case VK_FORMAT_R16G16_SINT: bits = 32; break;
    case VK_FORMAT_R16G16_SFLOAT: bits = 32; break;
    case VK_FORMAT_R16G16B16_UNORM: bits = 48; break;
    case VK_FORMAT_R16G16B16_SNORM: bits = 48; break;
    case VK_FORMAT_R16G16B16_USCALED: bits = 48; break;
    case VK_FORMAT_R16G16B16_SSCALED: bits = 48; break;
    case VK_FORMAT_R16G16B16_UINT: bits = 48; break;
    case VK_FORMAT_R16G16B16_SINT: bits = 48; break;
    case VK_FORMAT_R16G16B16_SFLOAT: bits = 48; break;
    case VK_FORMAT_R16G16B16A16_UNORM: bits = 64; break;
    case VK_FORMAT_R16G16B16A16_SNORM: bits = 64; break;
    case VK_FORMAT_R16G16B16A16_USCALED: bits = 64; break;
    case VK_FORMAT_R16G16B16A16_SSCALED: bits = 64; break;
    case VK_FORMAT_R16G16B16A16_UINT: bits = 64; break;
    case VK_FORMAT_R16G16B16A16_SINT: bits = 64; break;
    case VK_FORMAT_R16G16B16A16_SFLOAT: bits = 64; break;
    case VK_FORMAT_R32_UINT: bits = 32; break;
    case VK_FORMAT_R32_SINT: bits = 32; break;
    case VK_FORMAT_R32_SFLOAT: bits = 32; break;
    case VK_FORMAT_R32G32_UINT: bits = 64; break;
    case VK_FORMAT_R32G32_SINT: bits = 64; break;
    case VK_FORMAT_R32G32_SFLOAT: bits = 64; break;
    case VK_FORMAT_R32G32B32_UINT: bits = 96; break;
    case VK_FORMAT_R32G32B32_SINT: bits = 96; break;
    case VK_FORMAT_R32G32B32_SFLOAT: bits = 96; break;
    case VK_FORMAT_R32G32B32A32_UINT: bits = 128; break;
    case VK_FORMAT_R32G32B32A32_SINT: bits = 128; break;
    case VK_FORMAT_R32G32B32A32_SFLOAT: bits = 128; break;
    case VK_FORMAT_R64_UINT: bits = 64; break;
    case VK_FORMAT_R64_SINT: bits = 64; break;
    case VK_FORMAT_R64_SFLOAT: bits = 64; break;
    case VK_FORMAT_R64G64_UINT: bits = 128; break;
    case VK_FORMAT_R64G64_SINT: bits = 128; break;
    case VK_FORMAT_R64G64_SFLOAT: bits = 128; break;
    case VK_FORMAT_R64G64B64_UINT: bits = 192; break;
    case VK_FORMAT_R64G64B64_SINT: bits = 192; break;
    case VK_FORMAT_R64G64B64_SFLOAT: bits = 192; break;
    case VK_FORMAT_R64G64B64A64_UINT: bits = 256; break;
    case VK_FORMAT_R64G64B64A64_SINT: bits = 256; break;
    case VK_FORMAT_R64G64B64A64_SFLOAT: bits = 256; break;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32: bits = 32; break;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: bits = 32; break;
    case VK_FORMAT_D16_UNORM: bits = 16; break;
    case VK_FORMAT_X8_D24_UNORM_PACK32: bits =32 ; break;
    case VK_FORMAT_D32_SFLOAT: bits = 32; break;
    case VK_FORMAT_S8_UINT: bits = 8; break;
    case VK_FORMAT_D16_UNORM_S8_UINT: bits = 24; break;
    case VK_FORMAT_D24_UNORM_S8_UINT: bits = 32; break;
    case VK_FORMAT_D32_SFLOAT_S8_UINT: bits = 64; break;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK: bits = 64; break;
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK: bits = 64; break;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: bits = 64; break;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: bits = 64; break;
    case VK_FORMAT_BC2_UNORM_BLOCK: bits = 128; break;
    case VK_FORMAT_BC2_SRGB_BLOCK: bits = 128; break;
    case VK_FORMAT_BC3_UNORM_BLOCK: bits = 128; break;
    case VK_FORMAT_BC3_SRGB_BLOCK: bits = 128; break;
    case VK_FORMAT_BC4_UNORM_BLOCK: bits = 64; break;
    case VK_FORMAT_BC4_SNORM_BLOCK: bits = 64; break;
    case VK_FORMAT_BC5_UNORM_BLOCK: bits = 128; break;
    case VK_FORMAT_BC5_SNORM_BLOCK: bits = 128; break;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK: bits = 128; break;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK: bits = 128; break;
    case VK_FORMAT_BC7_UNORM_BLOCK: bits = 128; break;
    case VK_FORMAT_BC7_SRGB_BLOCK: bits = 128; break;
    // case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_EAC_R11_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_EAC_R11_SNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: bits = ; break;
    // case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: bits = ; break;
    // case VK_FORMAT_G8B8G8R8_422_UNORM: bits = ; break;
    // case VK_FORMAT_B8G8R8G8_422_UNORM: bits = ; break;
    // case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: bits = ; break;
    // case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM: bits = ; break;
    // case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: bits = ; break;
    // case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: bits = ; break;
    // case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM: bits = ; break;
    // case VK_FORMAT_R10X6_UNORM_PACK16: bits = ; break;
    // case VK_FORMAT_R10X6G10X6_UNORM_2PACK16: bits = ; break;
    // case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16: bits = ; break;
    // case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: bits = ; break;
    // case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: bits = ; break;
    // case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_R12X4_UNORM_PACK16: bits = ; break;
    // case VK_FORMAT_R12X4G12X4_UNORM_2PACK16: bits = ; break;
    // case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16: bits = ; break;
    // case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: bits = ; break;
    // case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: bits = ; break;
    // case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16: bits = ; break;
    // case VK_FORMAT_G16B16G16R16_422_UNORM: bits = ; break;
    // case VK_FORMAT_B16G16R16G16_422_UNORM: bits = ; break;
    // case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM: bits = ; break;
    // case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: bits = ; break;
    // case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: bits = ; break;
    // case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: bits = ; break;
    // case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM: bits = ; break;
    // case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: bits = ; break;
    // case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: bits = ; break;
    // case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: bits = ; break;
    // case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: bits = ; break;
    // case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: bits = ; break;
    // case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: bits = ; break;
    // case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: bits = ; break;
    // case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: bits = ; break;
    // case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT: bits = ; break;
    // case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT: bits = ; break;
    default:
      printf("WARNING: Unknown pixel format\n");
      return 0;
  }

  return static_cast<uint64_t>(
    width * height * (bits / 8)
  );
}