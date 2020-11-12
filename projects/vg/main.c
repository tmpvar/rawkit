#include <rawkit/rawkit.h>


void setup() {}
void loop() {
  rawkit_vg_t *vg = rawkit_vg_default();

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
