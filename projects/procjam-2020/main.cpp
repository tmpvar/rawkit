#include <rawkit/rawkit.h>

#define CPU_HOST
#include "glm/gtc/matrix_transform.hpp"
#include "shared.h"

void setup(){}
void loop() {
  vec2 window_dims(
    (float)rawkit_window_width(),
    (float)rawkit_window_height()
  );

  vec3 world_dims(128.0);
  rawkit_texture_t *blue_noise = rawkit_texture("blue-noise.ldr.png");

  rawkit_texture_t *world_texture = rawkit_texture_mem(
    "world-texture",
    world_dims.x,
    world_dims.y,
    world_dims.z,
    VK_FORMAT_R32G32B32A32_SFLOAT
  );

  rawkit_cpu_buffer_t *world_buffer = rawkit_cpu_buffer(
    "world-buffer",
    world_texture->options.size
  );

  // fill the world
  {
    vec4 *data = (vec4 *)world_buffer->data;
    vec3 half = vec3(world_dims) / 2.0f;
    for (uint32_t x=0; x<world_dims.x; x++) {
      for (uint32_t y=0; y<world_dims.y; y++) {
        for (uint32_t z=0; z<world_dims.z; z++) {
          vec3 p((float)x, (float)y, (float)z);
          uint64_t loc = x;
          loc += static_cast<uint64_t>(y * world_dims.x);
          loc += static_cast<uint64_t>(z * world_dims.x * world_dims.y);
          if (glm::distance(p, half) - half.x <= 0.0f) {

            data[loc].r = 1.0f;
            data[loc].g = 0.0f;
            data[loc].b = 1.0f;
            data[loc].a = 1.0f;
          } else {
            data[loc] = vec4(0.0);
          }
        }
      }
    }

    rawkit_texture_update_buffer(world_texture, world_buffer);
    world_texture->resource_version++;
  }


  rawkit_shader_t *world_shader = rawkit_shader(
    rawkit_file("shader/world.vert"),
    rawkit_file("shader/world.frag")
  );

  // setup world UBO
  struct world_ubo_t world_ubo = {};
  {
    mat4 proj = glm::perspective(
      80.0f,
      window_dims.x/window_dims.y,
      0.1f,
      100.0f
    );

    float now = (float)rawkit_now() * 0.1;
    vec3 eye(
      sin(now) * 2.0f,
      2.0f,
      cos(now) * 2.0f
    );

    mat4 view = glm::lookAt(
      eye,
      vec3(0.0f, 0.0f, 0.0f),
      vec3(0.0f, 1.0f, 0.0f)
    );

    world_ubo.worldToScreen = proj * view;
    world_ubo.world_dims = vec4(world_dims, 0.0f);
    world_ubo.eye = vec4(eye, 0.0f);
  }

  // render the world shader
  rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(world_shader);
  if (inst) {
    rawkit_shader_instance_param_ubo(inst, "UBO", &world_ubo);
    const rawkit_texture_sampler_t *nearest_sampler = rawkit_texture_sampler(
      inst->gpu,
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
    rawkit_shader_instance_param_texture(inst, "world_texture", world_texture, nearest_sampler);
    VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = window_dims.x,
      .height = window_dims.y
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

    vkCmdDraw(inst->command_buffer, 36, 1, 0, 0);

    rawkit_shader_instance_end(inst);
  }

}