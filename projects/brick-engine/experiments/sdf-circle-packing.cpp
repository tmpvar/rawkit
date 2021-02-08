#include <rawkit/rawkit.h>

#include "polygon.h"
#include "mouse.h"
#include "context-2d.h"

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

#if 1
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
    polygon->build_sdf();

    polygon->circle_pack();

    sb_push(state->polygons, polygon);
  }
}



void loop() {
  State *state = rawkit_hot_state("state", State);
  uint32_t polygon_count = sb_count(state->polygons);
  state->mouse.tick();

  Context2D ctx;

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

    ctx.strokeColor(rgb(0xFFFFFFFF));
    ctx.beginPath();
      ctx.moveTo(state->cutter.start);
      ctx.lineTo(state->cutter.end);
      ctx.stroke();


    if (perform_cut) {
      uint32_t cut_result_count = sb_count(state->polygon_cut_results);
      for (uint32_t i=0; i<cut_result_count; i++) {
        delete state->polygon_cut_results[i];
      }
      sb_reset(state->polygon_cut_results);

      for (uint32_t i=0; i<polygon_count; i++) {
        Polygon *polygon = state->polygons[i];
        polygon->render(ctx);

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
    polygon->render(ctx);

    // simulate a cut
    if (0) {
      ctx.save();
        ctx.translate(polygon->pos);
        ctx.rotate(polygon->rot);
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
              ctx.fillColor(rgb(0x0, 0x0, 0xff));
            } else if (side > 0.0f) {
              ctx.fillColor(rgb(0x0, 0xFF, 0x0));
            } else {
              ctx.fillColor(rgb(0xFF, 0, 0));
            }

            ctx.beginPath();
              ctx.arc(
                vec2(circle) + polygon->aabb.lb,
                abs(circle.w)// * 0.75f, // radius
              );
              ctx.fill();
          }
        }
      ctx.restore();
    }
  }


  uint32_t cut_result_count = sb_count(state->polygon_cut_results);
  igText("cut results: %u", cut_result_count);
  for (uint32_t i=0; i<cut_result_count; i++) {
    Polygon *polygon = state->polygon_cut_results[i];

    // simulate a cut
    {
      ctx.save();
        ctx.fillColor(hsl((float)i/(float)cut_result_count, 0.6f, 0.6f));
        ctx.translate(vec2(600, 100.0f + (float)i * 200.0f));
        // draw the packed circles
        {
          uint32_t circle_count = sb_count(polygon->circles);
          igText("  %u: %u circles", i, circle_count);
          for (uint32_t i=0; i<circle_count; i++) {
            vec4 circle = polygon->circles[i];
            ctx.beginPath();
              ctx.arc(vec2(circle), abs(circle.z));
              ctx.fill();
          }
        }
      ctx.restore();
    }
  }
}