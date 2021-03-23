#include "../state.h"

static rawkit_shader_options_t options = {
  .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
  .polygonMode = VK_POLYGON_MODE_LINE,
  .noVertexInput = true,
};

void renderer_lines(State *state, float tree_radius) {
  u32 c = sb_count(state->nodes);

  rawkit_shader_t *shader = rawkit_shader_opts(
    options,
    rawkit_file("./lines/lines.frag"),
    rawkit_file("./lines/lines.vert")
  );

  rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);
  if (inst) {
    rawkit_shader_instance_param_ssbo(inst, "NodePositions", state->node_positions_ssbo);
    rawkit_shader_instance_param_ssbo(inst, "NodeTree", state->nodes_ssbo);
    rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);

    VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = state->scene.screen_dims.x,
      .height = state->scene.screen_dims.y,
      .minDepth = 0.0,
      .maxDepth = 1.0
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

    vkCmdDraw(
      inst->command_buffer,
      24,
      c,
      0,
      0
    );

    rawkit_shader_instance_end(inst);
  }

}