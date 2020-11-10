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

  rawkit_glsl_t *glsl;

  VkCommandBuffer command_buffer;
} fill_rect_state_t;

void fill_rect(rawkit_gpu_t *gpu, const char *path, const fill_rect_options_t *options) {
  VkQueue queue = rawkit_vulkan_queue();
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

  const rawkit_file_t *file = rawkit_file(path);

  rawkit_shader_t *shader = rawkit_shader_ex(
    gpu,
    rawkit_window_frame_count(),
    rawkit_vulkan_renderpass(),
    1,
    &file
  );

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
    }
  }

  if (!shader || !shader->resource_version || !state->textures) {
    return;
  }

  uint32_t idx = rawkit_window_frame_index();
  VkCommandBuffer command_buffer = rawkit_shader_command_buffer(shader, idx);
  if (!command_buffer) {
    return;
  }

  // TODO: maybe only do this when the shader changes?
  {
    rawkit_shader_params_t p = {};
    rawkit_shader_params(p,
      rawkit_shader_texture("rawkit_output_image", &state->textures[idx])
    );
    rawkit_shader_apply_params(
      shader,
      idx,
      command_buffer,
      p
    );
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

    rawkit_shader_bind(
      shader,
      idx,
      command_buffer,
      options->params
    );

    {
      const rawkit_glsl_t *glsl = rawkit_shader_glsl(shader);
      const uint32_t *workgroup_size = rawkit_glsl_workgroup_size(glsl, 0);
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


void setup() {
  printf("setup %u\n", rawkit_window_frame_count());
}

struct triangle_uniforms {
  float color[4];
  float time;
};

void loop() {
  // TODO: this should be exposed some where
  rawkit_gpu_t gpu = {};
  gpu.physical_device = rawkit_vulkan_physical_device();
  gpu.device = rawkit_vulkan_device();
  gpu.pipeline_cache = rawkit_vulkan_pipeline_cache();

  {
    fill_rect_options_t options = {0};
    options.render_width = 128;
    options.render_height = 64;
    options.display_width = 512;
    options.display_height = 256;

    rawkit_shader_params(options.params,
      rawkit_shader_texture("input_image", rawkit_texture("box-gradient.png"))
    );

    fill_rect(&gpu, "basic.comp", &options);
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

    fill_rect(&gpu, "triangle.comp", &options);
  }

  {
    rawkit_shader_t *shader = rawkit_shader_ex(
      &gpu,
      rawkit_window_frame_count(),
      rawkit_vulkan_renderpass(),
      2,
      ((const rawkit_file_t *[]){
        rawkit_file("fullscreen.vert"),
        rawkit_file("fullscreen.frag")
      })
    );

    if (shader && shader->resource_version > 0) {
      VkCommandBuffer command_buffer = rawkit_vulkan_command_buffer();
      if (!command_buffer) {
        return;
      }

      rawkit_shader_params_t params = {};
      rawkit_shader_bind(
        shader,
        rawkit_window_frame_index(),
        command_buffer,
        params
      );

      VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)rawkit_window_width(),
        .height = (float)rawkit_window_height()
      };

      vkCmdSetViewport(
        command_buffer,
        0,
        1,
        &viewport
      );

      VkRect2D scissor = {};
      scissor.extent.width = viewport.width;
      scissor.extent.height = viewport.height;
      vkCmdSetScissor(
        command_buffer,
        0,
        1,
        &scissor
      );

      vkCmdDraw(command_buffer, 3, 1, 0, 0);
    }
  }
}
