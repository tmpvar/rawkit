#include <rawkit/rawkit.h>
#include <nanovg/nanovg.h>
int id = 0;

    #include "../../lib/rawkit-vg/src/nanovg/img/error-64x64.h"
void setup() {
  rawkit_vg_t *vg = rawkit_vg_default();
  uint32_t l = 64*64;
}
void loop() {
  rawkit_vg_t *vg = rawkit_vg_default();


  NVGpaint paint1 = rawkit_vg_texture(
    vg,
    0.0f,
    0.0f,
    128.0f,
    64.0f,
    0.0f,

    rawkit_texture("box-gradient.png"),
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

  rawkit_vg_draw_texture(vg, 300.0f, 300.0f, 128, 64, rawkit_texture("box-gradient.png"));


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
