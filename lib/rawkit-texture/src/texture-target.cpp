#include <rawkit/texture.h>
#include <rawkit/gpu.h>
#include <rawkit/window.h>

#include <array>
#include <vector>
#include <string>
using namespace std;

const std::vector<VkFormat> depthFormats = {
  VK_FORMAT_D32_SFLOAT_S8_UINT,
  VK_FORMAT_D32_SFLOAT,
  VK_FORMAT_D24_UNORM_S8_UINT,
  VK_FORMAT_D16_UNORM_S8_UINT,
  VK_FORMAT_D16_UNORM
};

VkFormat get_supported_depth_format(VkPhysicalDevice physical_device) {
  // Since all depth formats may be optional, we need to find a suitable depth format to use
  // Start with the highest precision packed format
  for (auto& format : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physical_device, format, &formatProps);
    // Format must support depth stencil attachment for optimal tiling
    if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      return format;
    }
  }

  return VK_FORMAT_UNDEFINED;
}

VkResult rawkit_texture_target_attachment(rawkit_texture_target_t *target, VkImageUsageFlagBits usage) {
  rawkit_texture_options_t options = {};
  options.gpu = target->gpu;
  options.width = target->width;
  options.height = target->height;
  options.source = nullptr;
  options.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | usage;
  switch (usage) {
    case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT: {
      string resource_name = string("mem+rawkit-texture-target+color://") + target->name;
      uint64_t id = rawkit_hash_resources(resource_name.c_str(), 1, (const rawkit_resource_t **)&target);
      target->color = rawkit_hot_resource_id(resource_name.c_str(), id, rawkit_texture_t);
      if (rawkit_resource_sources(target->color, target)) {
        return VK_SUCCESS;
      }

      options.format = VK_FORMAT_R8G8B8A8_SNORM;
      options.size = rawkit_texture_compute_size(
        target->width,
        target->height,
        options.format
      );

      if (rawkit_texture_init(target->color, options)) {
        target->color->resource_version++;
      }
      break;
    }

    case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT: {
      options.format = get_supported_depth_format(target->gpu->physical_device);
      string resource_name = string("mem+rawkit-texture-target+depth://") + target->name;
      uint64_t id = rawkit_hash_resources(resource_name.c_str(), 1, (const rawkit_resource_t **)&target);
      target->depth = rawkit_hot_resource_id(resource_name.c_str(), id, rawkit_texture_t);
      if (rawkit_resource_sources(target->depth, target)) {
        return VK_SUCCESS;
      }

      options.size = rawkit_texture_compute_size(
        target->width,
        target->height,
        options.format
      );

      if (rawkit_texture_init(target->depth, options)) {
        target->depth->resource_version++;
      }

      break;
    }

    default:
      printf("ERROR: rawkit_texture_target_attachment: unsupported attachment type");
      return VK_INCOMPLETE;
  }

  return VK_SUCCESS;
}

void rawkit_texture_target_reset(rawkit_texture_target_t *target) {
  if (!target) {
    return;
  }

  if (target->frame_buffer) {

  }

  if (target->render_pass) {

  }

  if (target->color) {
    rawkit_texture_destroy(target->color);
    target->color = nullptr;
  }

  if (target->depth) {
    rawkit_texture_destroy(target->depth);
    target->depth = nullptr;
  }

  return;
}

rawkit_texture_target_t *rawkit_texture_target_begin(
  rawkit_gpu_t *gpu,
  const char *name,
  uint32_t width,
  uint32_t height,
  bool depth
) {
  if (!gpu || !name || !width || !height) {
    return nullptr;
  }
  VkResult err;
  const uint32_t resource_name_max_len = 4096;
  char resource_name[resource_name_max_len];
  snprintf(resource_name, resource_name_max_len, "mem+rawkit-texture-target://%s", name);

  rawkit_texture_target_t *target = rawkit_hot_resource(resource_name, rawkit_texture_target_t);
  if (!target) {
    return nullptr;
  }

  target->name = name;
  target->gpu = gpu;

  // TODO: actually rebuild any existing framebuffers / renderpasses
  bool rebuild = (target->width != width || target->height != height);
  rebuild = rebuild || (depth != (target->depth != nullptr));

  target->width = width;
  target->height = height;

  // create the render pass & framebuffer
  if (rebuild) {
    printf("REBUILD: texture target (%s) (%ux%u) (depth=%i)\n", name, width, height, depth);
    rawkit_texture_target_reset(target);

    VkAttachmentReference color_reference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depth_reference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // setup the attached textures
    vector<VkAttachmentDescription> attachment_descriptions;

    // color attachment
    if (!target->color) {
      VkAttachmentDescription attachment = {};
      attachment.format = VK_FORMAT_R8G8B8A8_SNORM;
      attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachment_descriptions.push_back(attachment);

      // TODO: handle multiple color attachments?
      subpass_description.colorAttachmentCount = 1;
      subpass_description.pColorAttachments = &color_reference;

      err = rawkit_texture_target_attachment(target, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
      if (err) {
        printf("ERROR: rawkit_texture_target_begin: could not create color attachment (%i)\n", err);
        return target;
      }
    }

    // depth attachment
    if (depth && !target->depth) {
      VkAttachmentDescription attachment = {};
      attachment.format = get_supported_depth_format(target->gpu->physical_device);
      attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      attachment_descriptions.push_back(attachment);

      subpass_description.pDepthStencilAttachment = &depth_reference;
      err = rawkit_texture_target_attachment(target, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
      if (err) {
        printf("ERROR: rawkit_texture_target_begin: could not create depth attachment (%i)\n", err);
        return target;
      }
    } else if (!depth && target->depth) {
      rawkit_texture_destroy(target->depth);
      target->depth = nullptr;
    }

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
    {
      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      // TODO: depth?
      dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      dependencies[1].srcSubpass = 0;
      dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }

    // create the renderpass
    {
      VkRenderPassCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      info.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size());
      info.pAttachments = attachment_descriptions.data();
      info.subpassCount = 1;
      info.pSubpasses = &subpass_description;
      info.dependencyCount = static_cast<uint32_t>(dependencies.size());
      info.pDependencies = dependencies.data();

      err = vkCreateRenderPass(gpu->device, &info, nullptr, &target->render_pass);

      if (err) {
        printf("ERROR: rawkit_texture_target_begin: could not create render pass (%i)\n", err);
        return target;
      }
    }

    // create the framebuffer
    {
      vector<VkImageView> attachments = { target->color->image_view };
      if (depth) {
        attachments.push_back(target->depth->image_view);
      }

      VkFramebufferCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      info.renderPass = target->render_pass;
      info.attachmentCount = attachments.size();
      info.pAttachments = attachments.data();
      info.width = target->width;
      info.height = target->height;
      info.layers = 1;

      err = vkCreateFramebuffer(target->gpu->device, &info, nullptr, &target->frame_buffer);
      if (err) {
        printf("ERROR: rawkit_texture_target_begin: could not create framebuffer (%i)\n", err);
        return target;
      }
    }

    if (!target->command_pool) {
      VkCommandPoolCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      info.queueFamilyIndex = target->gpu->graphics_queue_family_index;
      err = vkCreateCommandPool(
        target->gpu->device,
        &info,
        target->gpu->allocator,
        &target->command_pool
      );
      if (err) {
        printf("ERROR: rawkit_texture_target_begin: could not create command pool (%i)\n", err);
        return target;
      }
    }

    target->resource_version++;
  }

  // create a command buffer for this instance
  uint32_t frame_count = rawkit_window_frame_count();
  if (!target->command_buffers && frame_count) {
    target->command_buffers = (VkCommandBuffer*)calloc(sizeof(VkCommandBuffer) * frame_count, 1);

    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = target->command_pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = frame_count;

    VkResult err = vkAllocateCommandBuffers(
      gpu->device,
      &info,
      target->command_buffers
    );

    if (err) {
      printf("ERROR: rawkit_texture_target_begin: could not create command buffer (%i)\n", err);
      return target;
    }
  }

  // begin the render pass
  {
    vector<VkClearValue> clearValues;

    VkClearValue clearColor = {};
    clearColor.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues.push_back(clearColor);

    if (depth) {
      VkClearValue clearDepth = {};
      clearDepth.depthStencil = { 1.0f, 0 };
      clearValues.push_back(clearDepth);
    }

    target->current_command_buffer = target->command_buffers[rawkit_window_frame_index()];

    //vkResetCommandBuffer(target->current_command_buffer, 0);
    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(target->current_command_buffer, &cmdBufferBeginInfo);

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = target->render_pass;
    renderPassBeginInfo.framebuffer = target->frame_buffer;
    renderPassBeginInfo.renderArea.extent.width = target->width;
    renderPassBeginInfo.renderArea.extent.height = target->height;
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(target->current_command_buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.width = (float)width;
    viewport.height = (float)height;
    vkCmdSetViewport(target->current_command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = (int32_t)width;
    scissor.extent.height = (int32_t)height;

    vkCmdSetScissor(target->current_command_buffer, 0, 1, &scissor);
  }

  return target;
}

void rawkit_texture_target_end(rawkit_texture_target_t *target) {
  if (!target) {
    return;
  }
  VkResult err;

  vkCmdEndRenderPass(target->current_command_buffer);

  err = vkEndCommandBuffer(target->current_command_buffer);
  if (err) {
    printf("ERROR: rawkit_texture_target_end: could not end command buffer (%i)\n", err);
    return;
  }

  VkSubmitInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  info.commandBufferCount = 1;
  info.pCommandBuffers = &target->current_command_buffer;

  err = vkQueueSubmit(
    target->gpu->graphics_queue,
    1,
    &info,
    VK_NULL_HANDLE
  );

  if (err) {
    printf("ERROR: rawkit_texture_target_end: could not submit command buffer to queue (%i)\n", err);
    return;
  }
}
