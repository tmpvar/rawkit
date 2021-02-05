#include <rawkit/rawkit.h>

#include "polygon.h"

struct State {
  Polygon **polygons;
};

void setup() {
  State *state = rawkit_hot_state("state", State);

  if (state->polygons) {
    uint32_t polygon_count = sb_count(state->polygons);
    for (uint32_t i=0; i<polygon_count; i++) {
      delete state->polygons[i];
      state->polygons[i] = nullptr;
    }
    sb_reset(state->polygons);
  }

  {
    Polygon *polygon = new Polygon("poly");
    polygon->pos = vec2(300.0, 300.0);

#if 0
    polygon->append(vec2(0.0, 0.0));
    polygon->append(vec2(400.0, 0.0));
    polygon->append(vec2(400.0, 200.0));
    polygon->append(vec2(0.0, 200.0));
#else
    polygon->append(vec2(0.0, 0.0));
    polygon->append(vec2(200.0, 0.0));
    // polygon->append(vec2(200.0, 200.0));
    // polygon->append(vec2(0.0, 200.0));
    polygon->append(vec2(400.0, 100.0));
    polygon->append(vec2(200.0, 200.0));
    polygon->append(vec2(200.0, 400.0));
    polygon->append(vec2(400.0, 150.0));
    polygon->append(vec2(100.0, 150.0));
    polygon->append(vec2(0.0, 200.0));
    polygon->append(vec2(0.0, 0.0));
#endif
    polygon->rebuild_sdf();

    polygon->circle_pack();

    sb_push(state->polygons, polygon);
  }
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  rawkit_vg_t *vg = rawkit_default_vg();

  uint32_t polygon_count = sb_count(state->polygons);
  for (uint32_t i=0; i<polygon_count; i++) {
    state->polygons[i]->render(vg);
  }


}