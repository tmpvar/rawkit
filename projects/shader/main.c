#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <rawkit/time.h>
#include <rawkit/file.h>
#include <rawkit/gpu.h>
#include <rawkit/hot.h>
#include <rawkit/glsl.h>
#include <rawkit/vulkan.h>


#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
  extern uint32_t rawkit_window_frame_index();
  extern uint32_t rawkit_window_frame_count();
#ifdef __cplusplus
}
#endif


#include <vulkan/vulkan.h>
#include <cimgui.h>
#include <string.h>


#include "rawkit-shader.h"
#include "rawkit-texture.h"

typedef struct fill_rect_options_t {
  uint32_t render_width;
  uint32_t render_height;
  uint32_t display_width;
  uint32_t display_height;

  rawkit_shader_params_t params;
} fill_rect_options_t;

typedef struct fill_rect_state_t {
  uint32_t width;
  uint32_t height;
  VkFormat format;
  rawkit_texture_t *textures;
  uint32_t texture_count;

  VkShaderModule shader_module;
  rawkit_shader_t *shaders;
  rawkit_glsl_t *glsl;
} fill_rect_state_t;

void fill_rect(const char *path, const fill_rect_options_t *options) {
  VkDevice device = rawkit_vulkan_device();
  if (device == VK_NULL_HANDLE) {
    printf("invalid vulkan device\n");
    return;
  }

  // TODO: hash options and compare

  VkResult err;

  uint32_t width = options->render_width;
  uint32_t height = options->render_height;
  // fall back to provided width/height
  uint32_t display_width = (options->display_width)
    ? options->display_width
    : width;

  uint32_t display_height = (options->display_height)
    ? options->display_height
    : height;

  char id[4096] = "rawkit::fill_rect::";
  strcat(id, path);
  fill_rect_state_t *state = rawkit_hot_state(id, fill_rect_state_t);
  if (!state) {
    printf("no state\n");
    return;
  }

  // rebuild the images if the user requests a resize
  if (state->width != width || state->height != height) {
    state->width = width;
    state->height = height;

    state->texture_count = rawkit_window_frame_count();
    if (!state->textures) {
      state->textures = (rawkit_texture_t *)calloc(
        state->texture_count * sizeof(rawkit_texture_t),
        1
      );

      if (state->textures == NULL) {
        return;
      }
    }

    rawkit_texture_options_t texture_options = {
      .width = width,
      .height = height,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
    };

    for (uint32_t idx=0; idx<state->texture_count; idx++) {
      rawkit_texture_init(&state->textures[idx], texture_options);

      if (state->shaders) {
        // update descriptor sets
        {
          VkDescriptorImageInfo imageInfo = {};
          imageInfo.sampler = state->textures[idx].sampler;
          imageInfo.imageView = state->textures[idx].image_view;
          imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

          rawkit_glsl_reflection_entry_t e = rawkit_glsl_reflection_entry(state->glsl, "rawkit_output_image");

          VkWriteDescriptorSet writeDescriptorSet = {};
          writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writeDescriptorSet.dstSet = state->shaders[idx].descriptor_sets[e.set];
          writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
          writeDescriptorSet.dstBinding = 0;
          writeDescriptorSet.pImageInfo = &imageInfo;
          writeDescriptorSet.descriptorCount = 1;
          vkUpdateDescriptorSets(
            device,
            1,
            &writeDescriptorSet,
            0,
            NULL
          );
        }
      }
    }
  }

  VkQueue queue = rawkit_vulkan_queue();
  // rebuild the actual shader if it changed
  {
    const rawkit_file_t *rawkit_shader_file = rawkit_file(path);
    if (rawkit_shader_file) {
      rawkit_glsl_t *glsl = rawkit_glsl_compile(
        path,
        (const char *)rawkit_shader_file->data,
        NULL
      );

      if (glsl) {
        bool rawkit_shader_changed = false;
        if (rawkit_glsl_valid(glsl)) {
          VkShaderModuleCreateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
          info.codeSize = rawkit_glsl_spirv_byte_len(glsl);
          info.pCode = rawkit_glsl_spirv_data(glsl);
          VkShaderModule module = VK_NULL_HANDLE;
          err = vkCreateShaderModule(
            rawkit_vulkan_device(),
            &info,
            NULL, //NULL /* v->Allocator */,
            &module
          );

          if (err == VK_SUCCESS) {
            if (state->shader_module != VK_NULL_HANDLE) {
              vkDestroyShaderModule(
                rawkit_vulkan_device(),
                state->shader_module,
                NULL
              );
            }
            state->shader_module = module;
            rawkit_shader_changed = true;
          } else {
            rawkit_glsl_destroy(glsl);
          }
        }

        // recreate the pipeline if the shader changed
        if (rawkit_shader_changed) {
          if (state->glsl) {
            rawkit_glsl_destroy(state->glsl);
          }
          state->glsl = glsl;
          // vkQueueWaitIdle(queue);

          // allocate a shader per frame buffer
          if (!state->shaders) {
            state->shaders = (rawkit_shader_t *)calloc(
              state->texture_count * sizeof(rawkit_shader_t),
              1
            );
          }

          // TODO: initialization of this type of shader (fill_rect) requires
          //       an output image per framebuffer. In order to make this work
          //       we will need to heap allocate some space for the incoming
          //       shader params as well as the output texture.
          for (uint32_t idx=0; idx < state->texture_count; idx++) {
            state->shaders[idx].shader_module = state->shader_module;
            rawkit_shader_init(
              glsl,
              &state->shaders[idx],
              &options->params,
              &state->textures[idx]
            );

            // update descriptor sets
            {
              VkDescriptorImageInfo imageInfo = {};
              imageInfo.sampler = state->textures[idx].sampler;
              imageInfo.imageView = state->textures[idx].image_view;
              imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

              rawkit_glsl_reflection_entry_t e = rawkit_glsl_reflection_entry(state->glsl, "rawkit_output_image");

              VkWriteDescriptorSet writeDescriptorSet = {};
              writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
              writeDescriptorSet.dstSet = state->shaders[idx].descriptor_sets[e.set];
              writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
              writeDescriptorSet.dstBinding = 0;
              writeDescriptorSet.pImageInfo = &imageInfo;
              writeDescriptorSet.descriptorCount = 1;
              vkUpdateDescriptorSets(
                device,
                1,
                &writeDescriptorSet,
                0,
                NULL
              );
            }
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

      // record new command buffer
      {
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

        VkImageSubresourceRange imageSubresourceRange;
        imageSubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        imageSubresourceRange.baseMipLevel   = 0;
        imageSubresourceRange.levelCount     = 1;
        imageSubresourceRange.baseArrayLayer = 0;
        imageSubresourceRange.layerCount     = 1;

        // VkClearColorValue clearColorValue = { 0.0, 0.0, 0.0, 0.0 };
        // vkCmdClearColorImage(
        //   command_buffer,
        //   state->textures[idx].image,
        //   VK_IMAGE_LAYOUT_GENERAL,
        //   &clearColorValue,
        //   1,
        //   &imageSubresourceRange
        // );

        vkCmdBindPipeline(
          state->shaders[idx].command_buffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          state->shaders[idx].pipeline
        );

        for (uint32_t i = 0; i<options->params.count; i++) {
          rawkit_shader_param_value_t val = rawkit_shader_param_value(&options->params.entries[i]);
          rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
            state->glsl,
            options->params.entries[i].name
          );

          if (entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER) {
            vkCmdPushConstants(
              state->shaders[idx].command_buffer,
              state->shaders[idx].pipeline_layout,
              VK_SHADER_STAGE_COMPUTE_BIT,
              entry.offset,
              val.len,
              val.buf
            );
          }
        }

        vkCmdBindDescriptorSets(
          state->shaders[idx].command_buffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          state->shaders[idx].pipeline_layout,
          0,
          state->shaders[idx].descriptor_set_count,
          state->shaders[idx].descriptor_sets,
          0,
          0
        );

        const uint32_t *workgroup_size = rawkit_glsl_workgroup_size(state->glsl);

        vkCmdDispatch(
          state->shaders[idx].command_buffer,
          width / workgroup_size[0],
          height / workgroup_size[1],
          1
        );

        vkEndCommandBuffer(state->shaders[idx].command_buffer);
      }

      // Submit compute commands
      {
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

        if (err != VK_SUCCESS) {
          printf("ERROR: unable to submit compute shader\n");
          return;
        }

        // render the actual image
        rawkit_texture_t *current_texture = &state->textures[idx];

        igImage(
          current_texture->imgui_texture,
          (ImVec2){ (float)display_width, (float)display_height},
          (ImVec2){ 0.0f, 0.0f }, // uv0
          (ImVec2){ 1.0f, 1.0f }, // uv1
          (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
          (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
        );
      }
    }
  }
}


void setup() {
  printf("setup %u\n", rawkit_window_frame_count());
}

void loop() {
  {
    fill_rect_options_t options = {0};
    options.render_width = 400;
    options.render_height = 400;

    rawkit_shader_push_constants(options.params,
      rawkit_shader_f32("time", (float)rawkit_now())
    );

    // Note: this is also a viable construction
    // options.push_constants_count = 1;
    // options.push_constants = (rawkit_shader_param_t[]){
    //   {
    //     .name = "time",
    //     .type = SHADER_PARAM_F32,
    //     .f32 = (float)rawkit_now()
    //   },
    // };

    fill_rect("basic.comp", &options);
  }

  {
    fill_rect_options_t options = {0};
    options.render_width = 400;
    options.render_height = 400;
    options.display_width = 400;
    options.display_height = 400;

    rawkit_shader_push_constants(options.params,
      rawkit_shader_f32("time", (float)rawkit_now()),
      rawkit_shader_f32("tx", 0.0f)
    );

    fill_rect("triangle.comp", &options);
  }
}
