#include <vulkan/vulkan.h>
#include <cimgui.h>

extern "C" {
  VkDevice rawkit_vulkan_device();
  VkPhysicalDevice rawkit_vulkan_physical_device();
  VkCommandBuffer rawkit_vulkan_command_buffer();
  VkCommandPool rawkit_vulkan_command_pool();
  ImTextureID rawkit_imgui_add_texture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);
  VkQueue rawkit_vulkan_queue();
}