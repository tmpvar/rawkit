#include <stdio.h>
#include <string.h>
#include <math.h>

#include <rawkit/rawkit.h>

typedef struct fill_rect_options_t {
  uint32_t render_width;
  uint32_t render_height;
  uint32_t display_width;
  uint32_t display_height;
  bool stretch;
  rawkit_shader_params_t params;
  const rawkit_texture_sampler_t *sampler;
} fill_rect_options_t;

typedef struct fill_rect_state_t {
  uint32_t width;
  uint32_t height;
  VkFormat format;
  uint32_t texture_count;
  rawkit_texture_t **textures;

  rawkit_glsl_t *glsl;

  VkCommandBuffer command_buffer;
} fill_rect_state_t;

const char *texture_names[] ={
  "rawkit::fill_rect::texture#0",
  "rawkit::fill_rect::texture#1",
  "rawkit::fill_rect::texture#2",
  "rawkit::fill_rect::texture#3",
  "rawkit::fill_rect::texture#4",
  "rawkit::fill_rect::texture#5",
};

void fill_rect(rawkit_gpu_t *gpu, const char *name, const char *path, const fill_rect_options_t *options) {
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
  strcat(id, "::");
  strcat(id, name);

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

  // TODO: in the rare event that the window frame count doesn't match the texture count
  //       then we need to rebuild.

  // rebuild the images if the user requests a resize
  if (state->width != width || state->height != height) {

    printf("REBUILD fill_rect(%s) (%u, %u) -> (%u, %u)\n", name, state->width, state->height, width, height);
    state->width = width;
    state->height = height;

    state->texture_count = rawkit_window_frame_count();
    if (!state->textures) {
      state->textures = (rawkit_texture_t **)calloc(
        state->texture_count * sizeof(rawkit_texture_t*),
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
      .gpu = rawkit_default_gpu(),
      .usage = (
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT
      )
    };

    // shader id, params id, texture index
    const uint8_t resource_ids_count = 3;
    uint64_t resource_ids[resource_ids_count] = {
      shader->resource_id,
      options->params.resource_id,
      0 // the texture index
    };

    for (uint64_t idx=0; idx<state->texture_count; idx++) {

      resource_ids[resource_ids_count-1] = idx;
      uint64_t id = rawkit_hash_composite(resource_ids_count, resource_ids);

      state->textures[idx] = rawkit_hot_resource_id(texture_names[idx], id, rawkit_texture_t);

      bool dirty = rawkit_resource_sources_array(
        (rawkit_resource_t *)state->textures[idx],
        1,
        (rawkit_resource_t **)&shader
      );

      if (rawkit_texture_init(state->textures[idx], texture_options)) {
        printf("initialized texture: %s\n", state->textures[idx]->resource_name);
        state->textures[idx]->resource_version++;
      }
    }
  }

  if (!shader || !shader->resource_version || !state->textures) {
    return;
  }

  uint32_t idx = rawkit_window_frame_index();

  VkCommandBuffer command_buffer = rawkit_gpu_create_command_buffer(gpu);
  if (!command_buffer) {
    printf("ERROR: fill_rect: could not create command buffer\n");
    return;
  }
  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  err = vkBeginCommandBuffer(
    command_buffer,
    &info
  );

  if (err != VK_SUCCESS) {
    printf("ERROR: could not begin command buffer");
    return;
  }

  // TODO: maybe only do this when the shader changes?
  {
    rawkit_shader_params_t p = {};
    rawkit_shader_params(p,
      rawkit_shader_texture("rawkit_output_image", state->textures[idx], NULL)
    );
    rawkit_shader_apply_params(
      shader,
      idx,
      command_buffer,
      p
    );
  }

  rawkit_texture_t *current_texture = state->textures[idx];

  // record new command buffer
  {
    // transition to a writable texture
    {
      VkImageMemoryBarrier barrier = {};
      barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      rawkit_texture_transition(
        current_texture,
        command_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        barrier
      );
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
        (uint32_t)fmaxf(ceilf(global[0] / local[0]), 1.0),
        (uint32_t)fmaxf(ceilf(global[1] / local[1]), 1.0),
        (uint32_t)fmaxf(ceilf(global[2] / local[2]), 1.0)
      );
    }

    // transition image to fragment shader readable
    {
      VkImageMemoryBarrier barrier = {};
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      // barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      rawkit_texture_transition(
        current_texture,
        command_buffer,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        barrier
      );
    }
  }

  // Submit compute commands
  {
    err = vkEndCommandBuffer(command_buffer);
    if (err != VK_SUCCESS) {
      printf("ERROR: vkEndCommandBuffer: failed %i\n", err);
      return;
    }

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkSubmitInfo computeSubmitInfo = {};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &command_buffer;
    computeSubmitInfo.pWaitDstStageMask = &waitStageMask;


    VkFence fence;
    {
      VkFenceCreateInfo create = {};
      create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      create.flags = 0;
      err = vkCreateFence(gpu->device, &create, gpu->allocator, &fence);
      if (err) {
        printf("ERROR: fill_rect: create fence failed (%i)\n", err);
        return;
      }
    }

    err = vkQueueSubmit(
      queue,
      1,
      &computeSubmitInfo,
      fence
    );

    rawkit_gpu_queue_command_buffer_for_deletion(gpu, command_buffer, fence, gpu->command_pool);

    if (err != VK_SUCCESS) {
      printf("ERROR: unable to submit compute shader\n");
      return;
    }

    ImTextureID texture = rawkit_imgui_texture(current_texture, options->sampler);
    if (!texture) {
      return;
    }

    ImVec2 uv1 = (ImVec2){
      (float)display_width/(float)width,
      (float)display_height/(float)height,
    };

    if (options->stretch) {
      uv1.x = 1.0f;
      uv1.y = 1.0f;
    }

    // render the actual image
    igImage(
      texture,
      (ImVec2){ (float)display_width, (float)display_height},
      (ImVec2){ 0.0f, 0.0f }, // uv0
      uv1,
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
  rawkit_gpu_t *gpu = rawkit_default_gpu();

  {
    fill_rect_options_t options = {0};
    options.render_width = 128;
    options.render_height = 64;
    options.display_width = 512;
    options.display_height = 256;
    options.stretch = true;
    options.sampler = rawkit_texture_sampler(gpu,
      VK_FILTER_NEAREST,
      VK_FILTER_NEAREST,
      VK_SAMPLER_MIPMAP_MODE_NEAREST,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      0.0f,
      false,
      0.0f,
      false,
      VK_COMPARE_OP_NEVER,
      0,
      1,
      VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
      false
    );

    rawkit_shader_params(options.params,
      rawkit_shader_texture(
        "input_image",
        rawkit_texture("box-gradient.png"),
        NULL
      )
    );

    fill_rect(gpu, "nearest", "basic.comp", &options);
  }

  // TODO: dedupe fill_rect resource by hashing shader params
  {
    fill_rect_options_t options = {0};
    options.render_width = 128;
    options.render_height = 64;
    options.display_width = 256;
    options.display_height = 128;
    options.sampler = rawkit_texture_sampler(gpu,
      VK_FILTER_LINEAR,
      VK_FILTER_LINEAR,
      VK_SAMPLER_MIPMAP_MODE_LINEAR,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      0.0f,
      false,
      0.0f,
      false,
      VK_COMPARE_OP_NEVER,
      0,
      1,
      VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
      false
    );

    rawkit_shader_params(options.params,
      rawkit_shader_texture(
        "input_image",
        rawkit_texture("box-gradient.png"),
        NULL
      )
    );

    fill_rect(gpu, "tiled", "basic.comp", &options);
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

    fill_rect(gpu, "triangle", "triangle.comp", &options);
  }

  {
    rawkit_shader_t *shader = rawkit_shader_ex(
      gpu,
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
