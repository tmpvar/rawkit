#include <rawkit/rawkit.h>
#include <nanovg/nanovg.h>
int id = 0;

const rawkit_texture_sampler_t *nearest = NULL;
void setup() {
  nearest = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_NEAREST,
    VK_FILTER_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
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
}



void loop() {
  rawkit_vg_t *vg = rawkit_vg_default();

  rawkit_texture_t *box_gradient = rawkit_texture("box-gradient.png");

  NVGpaint paint1 = rawkit_vg_texture(
    vg,
    0.0f,
    0.0f,
    128.0f,
    64.0f,
    0.0f,

    box_gradient,
    1.0f
  );

  NVGpaint notfound = rawkit_vg_texture(
    vg,
    0.0f,
    0.0f,
    64.0f,
    64.0f,
    0.0f,

    rawkit_texture("asdfasdf.png"),
    1.0f
  );

  rawkit_vg_begin_path(vg);
  rawkit_vg_rect(vg, 0.0f, 0.0f, rawkit_window_width(), rawkit_window_height());

  rawkit_vg_fill_paint(vg, notfound);
  rawkit_vg_fill(vg);

  rawkit_vg_stroke_color(vg, rawkit_vg_RGBA(255,255,255,255));
  rawkit_vg_stroke(vg);

  rawkit_vg_draw_texture(
    vg,
    300.0f,
    300.0f,
    128,
    64,
    box_gradient
  );

  static uint8_t frame = 0;
  const uint8_t frame_width = 8;

  const uint8_t frame_count = max(1, box_gradient->options.width)/frame_width;
  frame = (uint32_t)(floor(rawkit_now()*(float)(128/frame_width))) % max(1, frame_count);

  rawkit_vg_save(vg);
  rawkit_vg_translate(vg, 500.0, 100.0f);
  rawkit_vg_scale(vg, 8.0, 8.0f);

  rawkit_vg_draw_texture_rect(vg,
    (float)frame * frame_width,           // source x
    0.0f,                                 // source y
    (float)frame_width,                   // source w
    (float)box_gradient->options.height,  // source h
    0.0f,                                 // dest x
    0.0f,                                 // dest y
    box_gradient,                         // the texture to draw
    nearest                               // optional sampler
  );
  rawkit_vg_restore(vg);

  rawkit_vg_begin_path(vg);
    rawkit_vg_move_to(vg, 100.0f, 100.0f);
    rawkit_vg_line_to(vg, 100.0f, 200.0f);
    rawkit_vg_line_to(vg, 200.0f, 200.0f);
  rawkit_vg_close_path(vg);
  rawkit_vg_stroke_color(vg, rawkit_vg_RGBA(255,0,255,255));
  rawkit_vg_fill_color(vg, rawkit_vg_RGBA(255,255,0,255));
  rawkit_vg_stroke(vg);
  rawkit_vg_fill(vg);
}
