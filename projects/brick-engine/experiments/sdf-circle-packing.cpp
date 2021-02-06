#include <rawkit/rawkit.h>

#include "polygon.h"
#include "mouse.h"

struct Segment {
  vec2 start;
  vec2 end;
};

struct State {
  Polygon **polygons;
  Mouse mouse;
  Segment cutter;
  Polygon **polygon_cut_results;
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
    polygon->append(vec2(200.0, 0.0));
    polygon->append(vec2(200.0, 200.0));
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
  uint32_t polygon_count = sb_count(state->polygons);
  state->mouse.tick();
  rawkit_vg_t *vg = rawkit_default_vg();

  // construct a segment from screen center to the mouse

  {
    vec2 center = vec2(
      (float)rawkit_window_width(),
      (float)rawkit_window_height()
    ) * 0.5f;

    if (!state->mouse.was_down && state->mouse.down) {
      state->cutter.start = state->mouse.pos;
    }


    bool perform_cut = false;
    if (state->mouse.was_down || state->mouse.down) {
      state->cutter.end = state->mouse.pos;
      perform_cut = true;
    }

    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));
    rawkit_vg_begin_path(vg);
      rawkit_vg_move_to(vg, state->cutter.start.x, state->cutter.start.y);
      rawkit_vg_line_to(vg, state->cutter.end.x,   state->cutter.end.y);
    rawkit_vg_stroke(vg);


    if (perform_cut) {
      uint32_t cut_result_count = sb_count(state->polygon_cut_results);
      for (uint32_t i=0; i<cut_result_count; i++) {
        delete state->polygon_cut_results[i];
      }
      sb_reset(state->polygon_cut_results);

      for (uint32_t i=0; i<polygon_count; i++) {
        Polygon *polygon = state->polygons[i];
        polygon->render(vg);

        // cut
        {

          sprintf(polygon_tmp_str, "%s-split-left", polygon->name);
          Polygon *left = new Polygon(polygon_tmp_str);
          sprintf(polygon_tmp_str, "%s-split-right", polygon->name);
          Polygon *right = new Polygon(polygon_tmp_str);
          sprintf(polygon_tmp_str, "%s-split-isect", polygon->name);
          Polygon *isect = new Polygon(polygon_tmp_str);


          uint32_t circle_count = sb_count(polygon->circles);
          for (uint32_t i=0; i<circle_count; i++) {
            vec4 circle = polygon->circles[i];
            vec2 start = polygon->worldToLocal(state->cutter.start);
            vec2 end = polygon->worldToLocal(state->cutter.end);
            float side = orientation(
              start,
              end,
              vec2(circle)
            );

            vec2 closest = segment_closest_point(start, end, vec2(circle));

            if (distance(closest, vec2(circle)) < abs(circle.z)) {
              sb_push(isect->circles, circle);
            } else if (side > 0.0f) {
              sb_push(left->circles, circle);
            } else {
              sb_push(right->circles, circle);
            }
          }

          if (!sb_count(left->circles) || !sb_count(right->circles) || !sb_count(isect->circles)) {
            delete left;
            delete right;
            delete isect;
            continue;
          }

          if (sb_count(left->circles)) {
            sb_push(state->polygon_cut_results, left);
          }

          if (sb_count(right->circles)) {
            sb_push(state->polygon_cut_results, right);
          }

          if (sb_count(isect->circles)) {
            sb_push(state->polygon_cut_results, isect);
          }

        }
      }
    }
  }



  for (uint32_t i=0; i<polygon_count; i++) {
    Polygon *polygon = state->polygons[i];
    polygon->render(vg);

    // simulate a cut
    if (0) {
      rawkit_vg_save(vg);
        rawkit_vg_translate(vg, polygon->pos.x, polygon->pos.y);
        rawkit_vg_rotate(vg, polygon->rot);
        // draw the packed circles
        {
          uint32_t circle_count = sb_count(polygon->circles);
          for (uint32_t i=0; i<circle_count; i++) {
            vec4 circle = polygon->circles[i];
            vec2 start = polygon->worldToLocal(state->cutter.start);
            vec2 end = polygon->worldToLocal(state->cutter.end);
            float side = orientation(
              start,
              end,
              vec2(circle)
            );

            vec2 closest = segment_closest_point(start, end, vec2(circle));

            if (distance(closest, vec2(circle)) < abs(circle.z)) {
              rawkit_vg_fill_color(vg, rawkit_vg_RGB(0, 0, 0xff));
            } else if (side > 0.0f) {
              rawkit_vg_fill_color(vg, rawkit_vg_RGB(0, 0xFF, 0));
            } else {
              rawkit_vg_fill_color(vg, rawkit_vg_RGB(0xFF, 0, 0));
            }

            rawkit_vg_begin_path(vg);
              rawkit_vg_arc(
                vg,
                circle.x + polygon->aabb.lb.x,
                circle.y + polygon->aabb.lb.y,
                abs(circle.w),// * 0.75f, // radius
                0.0,
                6.283185307179586,
                1
              );
              rawkit_vg_fill(vg);
          }
        }
      rawkit_vg_restore(vg);
    }
  }


  uint32_t cut_result_count = sb_count(state->polygon_cut_results);
  igText("cut results: %u", cut_result_count);
  for (uint32_t i=0; i<cut_result_count; i++) {
    Polygon *polygon = state->polygon_cut_results[i];

    // simulate a cut
    {
      rawkit_vg_save(vg);
        rawkit_vg_fill_color(vg, rawkit_vg_HSL((float)i/(float)cut_result_count, 0.6f, 0.6f));
        rawkit_vg_translate(vg, 600, 100.0f + (float)i * 200.0f);
        // draw the packed circles
        {
          uint32_t circle_count = sb_count(polygon->circles);
          igText("  %u: %u circles", i, circle_count);
          for (uint32_t i=0; i<circle_count; i++) {
            vec4 circle = polygon->circles[i];
            rawkit_vg_begin_path(vg);
              rawkit_vg_arc(
                vg,
                circle.x,
                circle.y,
                abs(circle.w),// * 0.75f, // radius
                0.0,
                6.283185307179586,
                1
              );
              rawkit_vg_fill(vg);
          }
        }
      rawkit_vg_restore(vg);
    }
  }
}