#include <stdio.h>
#include <string.h>


#include <rawkit/rawkit.h>

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
  rawkit_shader_t *shaders;

  rawkit_glsl_t *glsl;

  VkCommandBuffer command_buffer;
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

  if (!state->command_buffer) {
    // Create a command buffer for compute operations
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = rawkit_vulkan_command_pool();
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    err = vkAllocateCommandBuffers(
      device,
      &info,
      &state->command_buffer
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: failed to allocate command buffers\n");
      return;
    }
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
        rawkit_shader_set_param(
          &state->shaders[idx],
          rawkit_shader_texture(
            "rawkit_output_image",
            &state->textures[idx]
          )
        );
      }
    }
  }

  VkQueue queue = rawkit_vulkan_queue();
  // rebuild the actual shader if it changed
  {
    const rawkit_file_t *rawkit_shader_file = rawkit_file(path);
    if (rawkit_shader_file) {

      rawkit_glsl_source_t source = {
        path,
        (const char *)rawkit_shader_file->data,
      };

      rawkit_glsl_t *glsl = rawkit_glsl_compile(
        1,
        &source,
        NULL
      );

      // recreate the pipeline if the shader changed and the new code compiled
      if (glsl) {
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
          state->shaders[idx].physical_device = rawkit_vulkan_physical_device();
          VkResult err = rawkit_shader_init(glsl, &state->shaders[idx]);

          if (err != VK_SUCCESS) {
            printf("ERROR: rawkit_shader_init failed (%i)\n", err);
            return;
          }

          // update descriptor sets
          rawkit_shader_set_param(
            &state->shaders[idx],
            rawkit_shader_texture(
              "rawkit_output_image",
              &state->textures[idx]
            )
          );
        }

        if (state->glsl) {
          rawkit_glsl_destroy(state->glsl);
          state->glsl = NULL;
        }
        state->glsl = glsl;
      }
    }

    if (state->shaders) {
      uint32_t idx = rawkit_window_frame_index();

      rawkit_shader_t *shader = &state->shaders[idx];
      if (!shader || !shader->glsl) {
        return;
      }

      VkCommandBuffer command_buffer = shader->command_buffer;
      if (!command_buffer) {
        return;
      }

      // record new command buffer
      {
        // TODO: we need to wait for the previous use of this command buffer to be complete
        //       don't use such a sledgehammer.
        // vkQueueWaitIdle(queue);

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
          command_buffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          shader->pipeline
        );

        for (uint32_t i = 0; i<options->params.count; i++) {
          rawkit_shader_param_t *param = &options->params.entries[i];
          rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
            shader->glsl,
            param->name
          );

          switch (entry.entry_type) {
            case RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER: {
              rawkit_shader_param_value_t val = rawkit_shader_param_value(param);
              rawkit_shader_update_ubo(
                shader,
                param->name,
                val.len,
                val.buf
              );

              break;
            }

            case RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER: {
              rawkit_shader_param_value_t val = rawkit_shader_param_value(param);
              vkCmdPushConstants(
                command_buffer,
                shader->pipeline_layout,
                VK_SHADER_STAGE_COMPUTE_BIT,
                entry.offset,
                val.len,
                val.buf
              );
              break;
            }

            case RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE: {
              // TODO: this is a nasty hack to get hot reloading textures working. The issue is that
              //       we need to update the texture descriptor for all shaders .. including the one
              //       that is currently in flight.

              // update descriptor sets
              rawkit_shader_set_param(
                &state->shaders[idx],
                rawkit_shader_texture(
                  param->name,
                  param->texture
                )
              );

              break;
            }


            default:
              printf("ERROR: unhandled entry type (%i) while setting shader params\n", entry.entry_type);
              break;
          }
        }

        vkCmdBindDescriptorSets(
          command_buffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          shader->pipeline_layout,
          0,
          shader->descriptor_set_count,
          shader->descriptor_sets,
          0,
          0
        );

        {
          const uint32_t *workgroup_size = rawkit_glsl_workgroup_size(shader->glsl, 0);
          float local[3] = {
            (float)workgroup_size[0],
            (float)workgroup_size[1],
            (float)workgroup_size[2],
          };

          float global[3] = {
            (float)width,
            (float)height,
            1,
          };

          vkCmdDispatch(
            command_buffer,
            (uint32_t)max(ceilf(global[0] / local[0]), 1.0),
            (uint32_t)max(ceilf(global[1] / local[1]), 1.0),
            (uint32_t)max(ceilf(global[2] / local[2]), 1.0)
          );
        }
        err = vkEndCommandBuffer(command_buffer);
        if (err != VK_SUCCESS) {
          printf("ERROR: vkEndCommandBuffer: failed %i\n", err);
          return;
        }

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

struct triangle_uniforms {
  float color[4];
  float time;
};

void loop() {
  {
    fill_rect_options_t options = {0};
    options.render_width = 128;
    options.render_height = 64;
    options.display_width = 512;
    options.display_height = 256;

    rawkit_shader_params(options.params,
      rawkit_shader_texture("input_image", rawkit_texture("box-gradient.png"))
    );

    fill_rect("basic.comp", &options);
  }

  {
    fill_rect_options_t options = {0};
    options.render_width = 400;
    options.render_height = 400;


    float time = (float)rawkit_now();

    struct triangle_uniforms ubo = {};
    ubo.time = time;
    ubo.color[0] = 1.0;
    ubo.color[1] = 0.0;
    ubo.color[2] = 1.0;
    ubo.color[3] = 1.0;

    rawkit_shader_params(options.params,
      rawkit_shader_ubo("UBO", &ubo)
    );

    fill_rect("triangle.comp", &options);
  }
}
