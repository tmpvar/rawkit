#include <rawkit/rawkit.h>


void setup() {}
void loop() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();

  uint32_t width = 320;
  uint32_t height = 200;
  rawkit_texture_target_t *texture_target = rawkit_texture_target_begin(
    gpu,
    "render-to-texture",
    width,
    height,
    false
  );

  rawkit_vg_t *vg = rawkit_vg_from_texture_target(texture_target); {
    rawkit_vg_begin_frame(vg, texture_target->command_buffer, (float)width, (float)height, 1.0f); {

      rawkit_vg_begin_path(vg);
        rawkit_vg_rect(vg, 0.0f, 0.0f, (float)width, (float)height);
        rawkit_vg_fill_color(vg, rawkit_vg_RGBAf(1.0f, 0.0f, 0.0f, 1.0f));
        rawkit_vg_fill(vg);
      rawkit_vg_begin_path(vg);
        rawkit_vg_rect(vg, 0.0f, 0.0f, (float)width * 0.5f, (float)height * 0.5f);
        rawkit_vg_fill_color(vg, rawkit_vg_RGBAf(1.0f, 0.0f, 1.0f, 1.0f));
        rawkit_vg_fill(vg);

      rawkit_vg_end_frame(vg);
    }
    rawkit_texture_target_end(texture_target);
  }

  ImTextureID texture = rawkit_imgui_texture(texture_target->color, texture_target->color->default_sampler);
  if (!texture) {
    return;
  }

  igImage(
    texture,
    (ImVec2){ (float)width, (float)height},
    (ImVec2){ 0.0f, 0.0f }, // uv0
    (ImVec2){ 1.0f, 1.0f }, // uv1
    (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
    (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
  );

  rawkit_vg_draw_texture(
    rawkit_default_vg(),
    0.0f,
    0.0f,
    320.0f,
    200.0f,
    texture_target->color
  );

}
