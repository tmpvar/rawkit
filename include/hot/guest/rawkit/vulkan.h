#include <vulkan/vulkan.h>
#include <cimgui.h>

#ifdef __cplusplus
extern "C" {
#endif
  VkDevice rawkit_vulkan_device();
  VkPhysicalDevice rawkit_vulkan_physical_device();
  // The main renderpass command buffer
  VkCommandBuffer rawkit_vulkan_command_buffer();
  VkCommandPool rawkit_vulkan_command_pool();
  ImTextureID rawkit_imgui_add_texture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);
  VkQueue rawkit_vulkan_queue();
  VkPipelineCache rawkit_vulkan_pipeline_cache();
  VkRenderPass rawkit_vulkan_renderpass();
  VkDescriptorPool rawkit_vulkan_descriptor_pool();
  // TODO: this is legacy, use rawkit_vulkan_find_queue instead
  int32_t rawkit_vulkan_queue_family();
  VkFramebuffer rawkit_current_framebuffer();
#ifdef __cplusplus
}
#endif