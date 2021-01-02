#include <rawkit/rawkit.h>

#include <glm/glm.hpp>
#define CPU_HOST
#include "shared.h"

const rawkit_texture_sampler_t *nearest_sampler;

struct State {
  Scene scene;
};

void setup() {
  State *state = rawkit_hot_state("state", State);

  nearest_sampler = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_LINEAR,
    VK_FILTER_LINEAR,
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

  state->scene.time = rawkit_now();
  state->scene.screen_dims.x = (float)rawkit_window_width();
  state->scene.screen_dims.y = (float)rawkit_window_height();

  // partitions
  {
    float dmin = 1.0f;
    float dmax = 100.0f;
    igSliderScalar("##paritions", ImGuiDataType_Float, &state->scene.partitions,  &dmin, &dmax, "partitions %f", 1.0f);
  }

  // time scale
  {
    float dmin = 1.0f;
    float dmax = 100.0f;
    igSliderScalar("##timescale", ImGuiDataType_Float, &state->scene.time_scale,  &dmin, &dmax, "timescale %f", 1.0f);
  }

  // time scale
  {
    float dmin = 1.0f;
    float dmax = 100.0f;
    igSliderScalar("##range", ImGuiDataType_Float, &state->scene.range,  &dmin, &dmax, "range %f", 1.0f);
  }

  ImVec2 mouse = {0.5, 0.5};
  igGetMousePos(&mouse);
  state->scene.mouse = vec4(
    mouse.x / state->scene.screen_dims.x,
    mouse.y / state->scene.screen_dims.y,
    0.0,
    0.0
  );

  igText("mpos(%f, %f)", state->scene.mouse.x, state->scene.mouse.y);
  auto viper_img = rawkit_texture("viper.png");

  // compute


  // render fullscreen
  if (1) {
    rawkit_shader_t *shader = rawkit_shader(
      rawkit_file("fullscreen.vert"),
      rawkit_file("fullscreen.frag")
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);

    if (inst) {
      rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
      rawkit_shader_instance_param_texture(inst, "logo", viper_img, nearest_sampler);

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