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

  rawkit_texture_t *blue_noise = rawkit_texture("blue-noise.ldr.png");
  rawkit_texture_t *world_data = rawkit_texture_mem(
    "world-data",
    128,
    128,
    128,
    VK_FORMAT_R8G8B8A8_UINT
  );

  rawkit_shader_t *world_shader = rawkit_shader(
    rawkit_file("shader/world.vert"),
    rawkit_file("shader/world.frag")
  );

  mat4 proj = glm::perspective(
    80.0f,
    window_dims.x/window_dims.y,
    0.1f,
    100.0f
  );

  mat4 view = glm::lookAt(
    vec3(-2.0f, 2.0f, -2.0f),
    vec3(0.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f)
  );

  struct world_ubo_t world_ubo = {};
  world_ubo.worldToScreen = proj * view;

  // render the world shader
  rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(world_shader);
  if (inst) {

    rawkit_shader_instance_param_ubo(inst, "UBO", &world_ubo);

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