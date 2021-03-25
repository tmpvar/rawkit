#include "../state.h"

void renderer_raytrace(State *state, float tree_radius) {
  u32 c = sb_count(state->nodes);

  rawkit_shader_t *shader = rawkit_shader(
    rawkit_file("raytrace/raytrace.comp")
  );

  vec2 dims = vec2(state->scene.screen_dims);
  rawkit_texture_t *color_output = rawkit_texture_mem(
    "renderer::raytrace::color-output",
    dims.x,
    dims.y,
    1,
    VK_FORMAT_R32G32B32A32_SFLOAT
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

  {
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