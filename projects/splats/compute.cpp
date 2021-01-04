/*
  splats/compute.cpp - render SDF surfaces with splats using
  https://github.com/m-schuetz/compute_rasterizer as a basis.


  OVERVIEW
    - generate an interesting SDF
    - run a compute shader over the bounds to find the zero crossings
    - for every zero crossing that faces the camera, add a point to the
      points list
    - run a compute shader over every point and atomicMin into an accumulation buffer
    - run a final compute shader to convert the accumulation buffer into a renderable
      texture
    - render the texture to the screen

*/

#define CPU_HOST
#include "shared.h"

#include <rawkit/rawkit.h>

struct State {
  Scene scene;
};

const rawkit_texture_sampler_t *nearest_sampler;

void setup() {
  State *state = rawkit_hot_state("state", State);

  nearest_sampler = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_NEAREST,
    VK_FILTER_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
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
}

void loop() {
  State *state = rawkit_hot_state("state", State);

  float now = rawkit_now();
  // setup state
  {
    state->scene.screen_dims = vec4(
      (float)rawkit_window_width(),
      (float)rawkit_window_height(),
      0.25,
      0
    );

    mat4 proj = glm::perspective(
      glm::radians(90.0f),
      state->scene.screen_dims.x / state->scene.screen_dims.y,
      0.1f,
      10000.0f
    );

    vec3 center = vec3(127.0);//vec3(scene_diameter * 0.5f) + 0.5f;


    float dist = 200;//(1.0 + length(center)) * 5.0;

    vec3 eye = center + vec3(
      sin(now * 0.1) * dist,
      dist * 0.125,
      cos(now * 0.1) * dist
    );

    // eye = vec3(dist, dist, dist);

    mat4 view = glm::lookAt(
      eye,
      center,
      vec3(0.0f, -1.0f, 0.0f)
    );

    state->scene.worldToScreen = proj * view;
    state->scene.brick_dims = vec4(16.0f);
    state->scene.eye = vec4(eye, 1.0f);
    state->scene.time = (float)rawkit_now();
  }

  rawkit_gpu_ssbo_t *framebuffer_ssbo = rawkit_gpu_ssbo(
    "FrameBuffer",
    sizeof(uint64_t) * static_cast<uint64_t>(state->scene.screen_dims.x * state->scene.screen_dims.y * state->scene.screen_dims.z)
  );

  rawkit_texture_t *framebuffer_texture = rawkit_texture_mem(
    "output",
    (uint32_t)ceil(state->scene.screen_dims.x * state->scene.screen_dims.z),
    (uint32_t)ceil(state->scene.screen_dims.y * state->scene.screen_dims.z),
    1,
    VK_FORMAT_R32G32B32A32_SFLOAT
  );

  rawkit_gpu_ssbo_t *points_ssbo = rawkit_gpu_ssbo("PointBuffer", sizeof(Point) * pow(256, 3));
  rawkit_gpu_ssbo_t *point_state_ssbo = rawkit_gpu_ssbo_ex(
    rawkit_default_gpu(),
    "PointState",
    sizeof(PointState),
    0,
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
  );

  // reset point list count
  {
    PointState s = { .dispatch = uvec3(0, 1, 1) };
    VkResult err = rawkit_gpu_ssbo_update(
      point_state_ssbo,
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      (void *)&s,
      sizeof(PointState)
    );
    points_ssbo->resource_version++;
    point_state_ssbo->resource_version++;
    framebuffer_ssbo->resource_version++;
  }

  // fill a point list
  {
    rawkit_shader_t *shader = rawkit_shader(
      rawkit_file("shader/fill.comp")
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
      rawkit_default_gpu(),
      shader,
      NULL,
      0
    );

    if (inst) {
      vkCmdFillBuffer(
        inst->command_buffer,
        points_ssbo->buffer->handle,
        0,
        points_ssbo->buffer->size,
        0x00
      );

      rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
      rawkit_shader_instance_param_ssbo(inst, "PointBuffer", points_ssbo);
      rawkit_shader_instance_param_ssbo(inst, "PointStateBuffer", point_state_ssbo);
      rawkit_shader_instance_dispatch_compute(
        inst,
        256,
        256,
        256
      );

      rawkit_shader_instance_end(inst);
    }
  }

  // splat
  {
    rawkit_shader_t *shader = rawkit_shader(
      rawkit_file("shader/splat.comp")
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
      rawkit_default_gpu(),
      shader,
      NULL,
      0
    );

    if (inst) {
      vkCmdFillBuffer(
        inst->command_buffer,
        framebuffer_ssbo->buffer->handle,
        0,
        framebuffer_ssbo->buffer->size,
        0xFFFFFFFF
      );


      rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
      rawkit_shader_instance_param_ssbo(inst, "PointBuffer", points_ssbo);
      rawkit_shader_instance_param_ssbo(inst, "FrameBuffer", framebuffer_ssbo);

      vkCmdDispatchIndirect(
        inst->command_buffer,
        point_state_ssbo->buffer->handle,
        0
      );

      rawkit_shader_instance_end(inst);
    }
  }

  // paint splats into texture
  {
    rawkit_shader_t *shader = rawkit_shader(
      rawkit_file("shader/paint.comp")
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
      rawkit_default_gpu(),
      shader,
      NULL,
      0
    );

    if (inst) {
      rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
      rawkit_shader_instance_param_ssbo(inst, "FrameBuffer", framebuffer_ssbo);
      rawkit_shader_instance_param_texture(inst, "output_image", framebuffer_texture, NULL);
      rawkit_shader_instance_dispatch_compute(
        inst,
        framebuffer_texture->options.width,
        framebuffer_texture->options.height,
        1
      );

      rawkit_shader_instance_end(inst);
    }
  }

  // present
  {
    rawkit_shader_t *shader = rawkit_shader(
      rawkit_file("shader/present.vert"),
      rawkit_file("shader/present.frag")
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);

    if (inst) {
      rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
      rawkit_shader_instance_param_ssbo(inst, "FrameBuffer", framebuffer_ssbo);
      rawkit_shader_instance_param_texture(inst, "framebuffer_texture", framebuffer_texture, nearest_sampler);

      VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)rawkit_window_width(),
        .height = (float)rawkit_window_height()
      };

      vkCmdSetViewport(
        inst->command_buffer,
        0,
        1,
        &viewport
      );

      VkRect2D scissor = {};
      scissor.extent.width = viewport.width;
      scissor.extent.height = viewport.height;
      vkCmdSetScissor(
        inst->command_buffer,
        0,
        1,
        &scissor
      );

      vkCmdDraw(inst->command_buffer, 3, 1, 0, 0);
      rawkit_shader_instance_end(inst);
    }
  }
}