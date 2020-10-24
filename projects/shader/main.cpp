#include <rawkit/gpu.h>
#include <rawkit/glsl.h>
#include <rawkit/vulkan.h>

#include <stdio.h>

extern "C" {
  extern uint32_t rawkit_window_frame_index();
  extern uint32_t rawkit_window_frame_count();
}


#include <vulkan/vulkan.h>
#include <cimgui.h>
#include <string.h>

#include <rawkit/file.h>
#include <rawkit/hot.h>
#include <rawkit/glsl.h>

typedef struct texture_t {
  ImTextureID imgui_texture;
  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;
  VkSampler sampler;
} texture_t;

typedef struct shader_t {
  VkCommandBuffer command_buffer;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorSet descriptor_set;
  VkWriteDescriptorSet write_descriptor_set;
  // reuse the graphics command pool for now and compare this with
  VkCommandPool command_pool;
} shader_t;

typedef struct fill_rect_state_t {
  uint32_t width;
  uint32_t height;
  VkFormat format;
  texture_t *textures;
  uint32_t texture_count;

  VkShaderModule shader_module;
  shader_t *shaders;
  uint32_t shader_count;
} fill_rect_state_t;

static uint32_t find_memory_type(VkMemoryPropertyFlags properties, uint32_t type_bits) {
    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(rawkit_vulkan_physical_device(), &prop);
    for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
        if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
            return i;
    return 0xFFFFFFFF; // Unable to find memoryType
}


void fill_rect(const char *path, uint32_t width, uint32_t height) {
  VkResult err;

  char id[4096] = "rawkit::fill_rect::";
  strcat(id, path);
  fill_rect_state_t *state = rawkit_hot_state(id, fill_rect_state_t);
  if (!state) {
    printf("no state\n");
    return;
  }

  VkDevice device = rawkit_vulkan_device();
  VkQueue queue = rawkit_vulkan_queue();
  VkPipelineCache pipeline_cache = rawkit_vulkan_pipeline_cache();

  // rebuild the images if the user requests a resize
  if (state->width != width || state->height != height) {
    printf("REBUILD\n");
    state->width = width;
    state->height = height;
    state->format = VK_FORMAT_R32G32B32A32_SFLOAT;
    if (device == VK_NULL_HANDLE) {
      printf("invalid vulkan device\n");
      return;
    }

    // cleanup existing resources
    for (uint32_t i=0; i<state->texture_count; i++) {
      printf("CLEANUP\n");
      texture_t *texture = &state->textures[i];
      vkDestroySampler(device, texture->sampler, NULL);
      vkDestroyImageView(device, texture->image_view, NULL);
      vkDestroyImage(device, texture->image, NULL);
      vkFreeMemory(device, texture->image_memory, NULL);
    }

    state->texture_count = rawkit_window_frame_count();
    if (!state->textures) {
      state->textures = (texture_t *)calloc(
        state->texture_count * sizeof(texture_t),
        1
      );

      if (state->textures == NULL) {
        return;
      }
    }

    for (uint32_t i=0; i<state->texture_count; i++) {
      printf("LOOP: %u of %u\n", i, state->texture_count);
      texture_t *texture = &state->textures[i];
      // create the image
      {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        // TODO: allow this to be provided
        info.format = state->format;
        info.extent.width = width;
        info.extent.height = height;
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
        info.format = state->format;
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

      printf("image sampler created\n");

      texture->imgui_texture = rawkit_imgui_add_texture(
        texture->sampler,
        texture->image_view,
        VK_IMAGE_LAYOUT_GENERAL
      );
      printf("texture created\n");
    }
  }

  // rebuild the actual shader if it changed
  {
    bool shader_changed = false;
    const rawkit_file_t *shader_file = rawkit_file(path);
    if (shader_file) {
      rawkit_glsl_t *glsl = rawkit_glsl_compile(
        path,
        (const char *)shader_file->data,
        NULL
      );

      if (glsl) {
        if (glsl->valid) {
          VkShaderModuleCreateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
          info.codeSize = glsl->bytes;
          info.pCode = glsl->data;
          VkShaderModule module = VK_NULL_HANDLE;
          err = vkCreateShaderModule(
            rawkit_vulkan_device(),
            &info,
            NULL, //NULL /* v->Allocator */,
            &module
          );

          rawkit_glsl_destroy(glsl);
          if (err == VK_SUCCESS) {
            if (state->shader_module != VK_NULL_HANDLE) {
              vkDestroyShaderModule(
                rawkit_vulkan_device(),
                state->shader_module,
                NULL
              );
            }
            state->shader_module = module;
            shader_changed = true;
          }
        }
      }
    }

    // recreate the pipeline if the shader changed
    if (shader_changed) {
      vkQueueWaitIdle(queue);

      // allocate a shader per frame buffer
      // TODO: texture_count != shader_count
      if (!state->shaders) {
        state->shader_count = state->texture_count;
        state->shaders = (shader_t *)calloc(
          state->shader_count * sizeof(shader_t),
          1
        );

        for (uint32_t shader_idx=0; shader_idx < state->shader_count; shader_idx++) {
          VkCommandPoolCreateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
          info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
          info.queueFamilyIndex = rawkit_vulkan_queue_family();
          err = vkCreateCommandPool(
            device,
            &info,
            NULL,
            &state->shaders[shader_idx].command_pool
          );

          if (err != VK_SUCCESS) {
            printf("ERROR: could not create command pool\n");
            return;
          }
        }
      }

      // rebuild descriptor sets
      {
        for (uint32_t shader_idx=0; shader_idx < state->shader_count; shader_idx++) {

          VkDescriptorSetLayoutBinding setLayoutBindings = {};
          setLayoutBindings.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
          setLayoutBindings.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
          setLayoutBindings.binding = 0;
          setLayoutBindings.descriptorCount = 1;

          VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
          descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          descriptorSetLayoutCreateInfo.pBindings = &setLayoutBindings;
          descriptorSetLayoutCreateInfo.bindingCount = 1;

          err = vkCreateDescriptorSetLayout(
            device,
            &descriptorSetLayoutCreateInfo,
            nullptr,
            &state->shaders[shader_idx].descriptor_set_layout
          );

          if (err != VK_SUCCESS) {
            printf("ERROR: could note create descriptor set layout\n");
            return;
          }

          VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
          descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          descriptorSetAllocateInfo.descriptorPool = rawkit_vulkan_descriptor_pool();
          descriptorSetAllocateInfo.pSetLayouts = &state->shaders[shader_idx].descriptor_set_layout;
          descriptorSetAllocateInfo.descriptorSetCount = 1;

          err = vkAllocateDescriptorSets(
            device,
            &descriptorSetAllocateInfo,
            &state->shaders[shader_idx].descriptor_set
          );

          if (err != VK_SUCCESS) {
            printf("ERROR: unable to allocate descriptor sets\n");
            return;
          }

          VkDescriptorImageInfo imageInfo = {};
          imageInfo.sampler = state->textures[shader_idx].sampler;
          imageInfo.imageView = state->textures[shader_idx].image_view;
          imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

          VkWriteDescriptorSet writeDescriptorSet = {};
          writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writeDescriptorSet.dstSet = state->shaders[shader_idx].descriptor_set;
          writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
          writeDescriptorSet.dstBinding = 0;
          writeDescriptorSet.pImageInfo = &imageInfo;
          writeDescriptorSet.descriptorCount = 1;
          vkUpdateDescriptorSets(
            device,
            1,
            &writeDescriptorSet, //&state->shaders[shader_idx].descriptor_set,
            0,
            NULL
          );

          VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
          pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
          pipelineLayoutCreateInfo.setLayoutCount = 1;
          pipelineLayoutCreateInfo.pSetLayouts = &state->shaders[shader_idx].descriptor_set_layout;
          err = vkCreatePipelineLayout(
            device,
            &pipelineLayoutCreateInfo,
            nullptr,
            &state->shaders[shader_idx].pipeline_layout
          );

          if (err != VK_SUCCESS) {
            printf("ERROR: unable to create pipeline layout\n");
            return;
          }

          VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
          pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
          pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
          pipelineShaderStageCreateInfo.module = state->shader_module;
          pipelineShaderStageCreateInfo.pName = "main";

          VkComputePipelineCreateInfo computePipelineCreateInfo = {};
          computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
          computePipelineCreateInfo.layout = state->shaders[shader_idx].pipeline_layout;
          computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;

          err = vkCreateComputePipelines(
            device,
            pipeline_cache,
            1,
            &computePipelineCreateInfo,
            nullptr,
            &state->shaders[shader_idx].pipeline
          );

          if (err != VK_SUCCESS) {
            printf("ERROR: failed to create compute pipelines\n");
          }

          if (!state->shaders[shader_idx].command_buffer) {
            // Create a command buffer for compute operations
            VkCommandBufferAllocateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            info.commandPool = state->shaders[shader_idx].command_pool;
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            info.commandBufferCount = 1;

            err = vkAllocateCommandBuffers(
              device,
              &info,
              &state->shaders[shader_idx].command_buffer
            );

            if (err != VK_SUCCESS) {
              printf("ERROR: failed to allocate command buffers\n");
              return;
            }
          }

          // record new command buffer
          {
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            err = vkBeginCommandBuffer(
              state->shaders[shader_idx].command_buffer,
              &info
            );

            if (err != VK_SUCCESS) {
              printf("ERROR: could not begin command buffer");
              return;
            }

            vkCmdBindPipeline(
              state->shaders[shader_idx].command_buffer,
              VK_PIPELINE_BIND_POINT_COMPUTE,
              state->shaders[shader_idx].pipeline
            );

            vkCmdBindDescriptorSets(
              state->shaders[shader_idx].command_buffer,
              VK_PIPELINE_BIND_POINT_COMPUTE,
              state->shaders[shader_idx].pipeline_layout,
              0,
              1,
              &state->shaders[shader_idx].descriptor_set,
              0,
              0
            );

            vkCmdDispatch(
              state->shaders[shader_idx].command_buffer,
              width,
              height,
              1
            );

            vkEndCommandBuffer(state->shaders[shader_idx].command_buffer);
          }
        }
      }
    }

    if (state->shaders) {
      uint32_t idx = rawkit_window_frame_index();
      VkCommandBuffer command_buffer = state->shaders[idx].command_buffer;
      if (!command_buffer) {
        return;
      }

      VkCommandBufferBeginInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      err = vkBeginCommandBuffer(
        command_buffer,
        &info
      );

      if (err != VK_SUCCESS) {
        printf("ERROR: could not begin command buffer");
        return;
      }

      vkCmdBindPipeline(
        command_buffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        state->shaders[idx].pipeline
      );

      vkCmdBindDescriptorSets(
        command_buffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        state->shaders[idx].pipeline_layout,
        0,
        1,
        &state->shaders[idx].descriptor_set,
        0,
        0
      );

      vkCmdDispatch(
        command_buffer,
        width,// / 16,
        height,// / 16,
        1
      );

      vkEndCommandBuffer(command_buffer);

      // Submit compute commands
      VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      VkSubmitInfo computeSubmitInfo = {};
      computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      computeSubmitInfo.commandBufferCount = 1;
      computeSubmitInfo.pCommandBuffers = &command_buffer;
      computeSubmitInfo.pWaitDstStageMask = &waitStageMask;

      err = vkQueueSubmit(
        queue,
        1,
        &computeSubmitInfo,
        VK_NULL_HANDLE
      );
      vkQueueWaitIdle(queue);
      if (err != VK_SUCCESS) {
        printf("ERROR: unable to submit compute shader\n");
        return;
      }
    }
  }

  // render the actual image
  texture_t *current_texture = &state->textures[rawkit_window_frame_index()];

  igImage(
    current_texture->imgui_texture,
    { (float)state->width, (float)state->height},
    { 0.0f, 0.0f }, // uv0
    { 1.0f, 1.0f }, // uv1
    {1.0f, 1.0f, 1.0f, 1.0f}, // tint color
    {1.0f, 1.0f, 1.0f, 1.0f} // border color
  );
}


void setup() {
  printf("setup %u\n", rawkit_window_frame_count());
}

void loop() {
  // shader_rect_t *rect = shader_rect("basic.frag", "basic.vert");
  fill_rect("basic.comp", 400, 400);

   //printf("frame %u\n", rawkit_window_frame_index());
}
