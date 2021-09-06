#pragma once

#include <vulkan/vulkan.h>
#include <cimgui.h>

#ifdef __cplusplus
extern "C" {
#endif
  #ifdef RAWKIT_WORKER
    static inline VkDevice rawkit_vulkan_device() { return VK_NULL_HANDLE; }
    static inline VkPhysicalDevice rawkit_vulkan_physical_device() { return VK_NULL_HANDLE; }

    // The main renderpass command buffer
    static inline VkCommandBuffer rawkit_vulkan_command_buffer() { return VK_NULL_HANDLE; }
    static inline VkCommandPool rawkit_vulkan_command_pool() { return VK_NULL_HANDLE; }

    // the current frame's main command buffer
    static inline VkQueue rawkit_vulkan_queue() { return VK_NULL_HANDLE; }
    static inline VkPipelineCache rawkit_vulkan_pipeline_cache() { return VK_NULL_HANDLE; }
    static inline VkRenderPass rawkit_vulkan_renderpass() { return VK_NULL_HANDLE; }
    static inline VkDescriptorPool rawkit_vulkan_descriptor_pool() { return VK_NULL_HANDLE; }

    static inline VkFramebuffer rawkit_current_framebuffer() { return VK_NULL_HANDLE; }
    static inline void rawkit_renderpass_timeline_semaphore(VkSemaphore semaphore, u64 wait, u64 signal) {}

  #else
    VkDevice rawkit_vulkan_device();
    VkPhysicalDevice rawkit_vulkan_physical_device();
    // The main renderpass command buffer
    VkCommandBuffer rawkit_vulkan_command_buffer();
    VkCommandPool rawkit_vulkan_command_pool();
    // the current frame's main command buffer
    VkQueue rawkit_vulkan_queue();
    VkPipelineCache rawkit_vulkan_pipeline_cache();
    VkRenderPass rawkit_vulkan_renderpass();
    VkDescriptorPool rawkit_vulkan_descriptor_pool();

    VkFramebuffer rawkit_current_framebuffer();

    void rawkit_renderpass_timeline_semaphore(VkSemaphore semaphore, u64 wait, u64 signal);

  #endif
#ifdef __cplusplus
}
#endif