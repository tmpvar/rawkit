#include <stdlib.h>

#include <rawkit/gpu.h>
#include <rawkit/hash.h>
#include <rawkit/image.h>
#include <rawkit/texture.h>

#include <string>
using namespace std;

VkDeviceSize computeTextureSize(uint32_t width, uint32_t height, VkFormat format) {
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
      bits = 0;
  }

  if (bits == 0) {
    printf("WARNING: Unknown pixel format\n");
    return 0;
  }

  return static_cast<VkDeviceSize>(
    width * height * (bits / 8)
  );
}

void rawkit_texture_destroy(rawkit_texture_t *texture) {
  if (!texture) {
    return;
  }

  if (!texture->options.gpu) {
    return;
  }
  rawkit_gpu_t *gpu = texture->options.gpu;

  // TODO: ensure none of these resources are currently in use!
  if (gpu->device != VK_NULL_HANDLE) {
    if (texture->image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(gpu->device, texture->image_view, gpu->allocator);
      texture->image_view = VK_NULL_HANDLE;
    }

    if (texture->image != VK_NULL_HANDLE) {
      vkDestroyImage(gpu->device, texture->image, gpu->allocator);
      texture->image = VK_NULL_HANDLE;
    }

    if (texture->image_memory != VK_NULL_HANDLE) {
      vkFreeMemory(gpu->device, texture->image_memory, gpu->allocator);
      texture->image_memory = VK_NULL_HANDLE;
    }

    if (texture->source_cpu_buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(gpu->device, texture->source_cpu_buffer, gpu->allocator);
      texture->source_cpu_buffer = VK_NULL_HANDLE;
    }

    if (texture->source_cpu_buffer_memory != VK_NULL_HANDLE) {
      vkFreeMemory(gpu->device, texture->source_cpu_buffer_memory, gpu->allocator);
      texture->source_cpu_buffer_memory = VK_NULL_HANDLE;
    }
  }
}

bool rawkit_texture_init(rawkit_texture_t *texture, const rawkit_texture_options_t options) {
  rawkit_gpu_t *gpu = options.gpu;
  VkDevice device = gpu->device;
  VkQueue queue = rawkit_vulkan_find_queue(gpu, VK_QUEUE_GRAPHICS_BIT);
  VkCommandPool command_pool = gpu->command_pool;
  VkPipelineCache pipeline_cache = gpu->pipeline_cache;

  if (!texture->default_sampler) {
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
  }

  if (!options.width || !options.height) {
    return false;
  }

  // rebuild the images if the user requests a resize
  if (texture->options.width != options.width || texture->options.height != options.height) {
    // cleanup existing resources
    rawkit_texture_destroy(texture);
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

  // create a command buffer
  if (texture->command_buffer == VK_NULL_HANDLE) {
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
  if (texture->image == VK_NULL_HANDLE) {
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
      gpu->allocator,
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
      gpu->allocator,
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
  if (texture->image_view == VK_NULL_HANDLE) {
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

  // create the upload buffer
  if (texture->source_cpu_buffer_memory == VK_NULL_HANDLE && options.size) {
    // create the upload buffer
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = options.size;
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
  options.size = computeTextureSize(
    options.width,
    options.height,
    options.format
  );
  // TODO: init doesn't do anything with the source! remove me
  options.gpu = gpu;

  vkDeviceWaitIdle(device);
  // cache miss
  // TODO: cleanup existing resources!!!!!!
  if (!rawkit_texture_init(texture, options)) {
    return texture;
  }

  // upload the new image data
  if (texture->source_cpu_buffer_memory) {
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

