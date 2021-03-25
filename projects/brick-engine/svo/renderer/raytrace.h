#include "../state.h"

void renderer_raytrace(State *state, float tree_radius) {
  u32 c = sb_count(state->nodes);


  vec2 dims = vec2(state->scene.screen_dims);
  rawkit_texture_t *color_output = rawkit_texture_mem(
    "renderer::raytrace::color-output",
    dims.x,
    dims.y,
    1,
    VK_FORMAT_R32G32B32A32_SFLOAT
  );

  // raytrace into a texture
  {
    rawkit_shader_t *shader = rawkit_shader(
      rawkit_file("raytrace/raytrace.comp")
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
      rawkit_default_gpu(),
      shader,
      NULL,
      0
    );

    if (inst) {
      rawkit_shader_instance_param_ssbo(inst, "NodePositions", state->node_positions_ssbo);
      rawkit_shader_instance_param_ssbo(inst, "NodeTree", state->nodes_ssbo);
      rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
      rawkit_shader_instance_param_texture(inst, "color", color_output, nullptr);


      rawkit_shader_instance_dispatch_compute(
        inst,
        dims.x,
        dims.y,
        1
      );

      rawkit_shader_instance_end(inst);
    }
  }

  // render the texture to a full screen surface
  {
    rawkit_shader_t *shader = rawkit_shader(
      rawkit_file("raytrace/fullscreen.vert"),
      rawkit_file("raytrace/fullscreen.frag")
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);
    if (inst) {
      rawkit_shader_instance_param_texture(inst, "tex_color", color_output, nullptr);

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
      vkCmdDraw(inst->command_buffer, 3, 1, 0, 0);
      rawkit_shader_instance_end(inst);
    }
  }


  // render color texture in imgui
  if (0) {
    float display_width = 460.0f;
    float display_height = display_width / (dims.x / dims.y);
    ImTextureID texture = rawkit_imgui_texture(color_output, color_output->default_sampler);
    if (!texture) {
      return;
    }


    // render the actual image
    igImage(
      texture,
      (ImVec2){ (float)display_width, (float)display_height},
      (ImVec2){ 0.0f, 0.0f }, // uv0
      (ImVec2){ 1.0f, 1.0f }, // uv0
      (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
      (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
    );
  }

}