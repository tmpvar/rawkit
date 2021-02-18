#include <rawkit/rawkit.h>
#include "context-2d.h"
#include "camera-2d.h"
#include "sdf.h"
#include "blob.h"
#include "mouse.h"
#include "segment.h"

struct State {
  Mouse mouse;
  Blob **blobs;
  Camera2D camera;
  Segment *cuts;

  vec2 *polyline_slicer;
};


void setup () {
  State *state = rawkit_hot_state("state", State);

  sb_reset(state->cuts);

  state->camera.setup();
  // add a blob
  {

    uint32_t blob_count = sb_count(state->blobs);
    if (blob_count) {
      for (uint32_t i=0; i<blob_count; i++) {
        delete state->blobs[i];
      }
      sb_reset(state->blobs);
    }

    Polygon *polygon = new Polygon("poly");
    polygon->pos = vec2(300.0, 300.0);
    // polygon->append(vec2(0.0, 0.0));
    // polygon->append(vec2(200.0, 0.0));
    // polygon->append(vec2(200.0, 200.0));
    // polygon->append(vec2(0.0, 200.0));

    polygon->append(vec2(0.0, 0.0));
    polygon->append(vec2(200.0, 0.0));
    polygon->append(vec2(200.0, 200.0));
    polygon->append(vec2(150.0, 200.0));
    polygon->append(vec2(150.0, 50.0));
    polygon->append(vec2(0.0, 200.0));

    // polygon->append(vec2(0.0, 0.0));
    // polygon->append(vec2(200.0, 0.0));
    // // polygon->append(vec2(200.0, 200.0));
    // // polygon->append(vec2(0.0, 200.0));
    // polygon->append(vec2(700.0, 100.0));
    // polygon->append(vec2(200.0, 200.0));
    // polygon->append(vec2(200.0, 400.0));
    // polygon->append(vec2(400.0, 150.0));
    // polygon->append(vec2(100.0, 150.0));
    // polygon->append(vec2(0.0, 200.0));
    // polygon->append(vec2(0.0, 0.0));

    Blob *blob = new Blob(
      "square",
      vec2(polygon->aabb.width(), polygon->aabb.height())
    );

    blob->pos = polygon->pos;
    polygon->build_sdf(blob->sdf);
    delete polygon;

    blob->compute_center_of_mass();
    blob->pos += blob->center_of_mass;
    // blob->rot = 0.9f;
    blob->circle_pack();
    blob->circle_graph();
    blob->extract_islands();
    sb_push(state->blobs, (Blob *)blob);

    // sb_push(state->blobs, (Blob *)new CircleBlob(
    //   "circle",
    //   100.0f
    // ));

    // slice
    if (0) {

      vec2 polyline[] = {
        blob->gridToWorld(vec2(100.0, 0.0)),
        blob->gridToWorld(vec2(100.0, 100.0)),
        blob->gridToWorld(vec2(0.0, 110.0)),
        blob->gridToWorld(vec2(150.0, 200.0)),
      };

      Blob **blobs = blob->slice_with_polyline(
        3,
        polyline
      );

      if (blobs != nullptr) {
        uint32_t blob_count = sb_count(blobs);
        for (uint32_t i=0; i<blob_count; i++) {
          if (!blobs[i]) {
            continue;
          }

          blobs[i]->circle_pack();
          blobs[i]->circle_graph();
          Blob **islands = blobs[i]->extract_islands();

          uint32_t island_count = sb_count(islands);
          for (uint32_t j=0; j<island_count; j++) {
            if (!islands[j]) {
              continue;
            }
            sb_push(state->blobs, islands[j]);
          }
          delete blobs[i];
        }

        sb_free(blobs);
      }

      printf("done slicing\n");
    }
  }
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  state->mouse.tick();

  Context2D ctx;


  state->camera.tick(
    igIsMouseDown(ImGuiMouseButton_Middle),
    state->mouse.pos,
    state->mouse.wheel * 0.1f
  );
  igText("camera translation(%f, %f)", state->camera.translation.x, state->camera.translation.y);
  state->camera.begin();

  if (state->mouse.down) {
    if (!state->mouse.was_down) {
      sb_reset(state->polyline_slicer);
    }

    sb_push(state->polyline_slicer, state->camera.pointToWorld(state->mouse.pos));

    u32 c = sb_count(state->polyline_slicer);

    ctx.strokeColor(rgb(0xFFFFFFFF));
    ctx.beginPath();
    ctx.moveTo(state->polyline_slicer[0]);
    for (u32 i=1; i<c; i++) {
      ctx.lineTo(state->polyline_slicer[i]);
    }

    ctx.stroke();
  }

  ctx.fillColor(rgb(0x99, 0x99, 0x99));
  ctx.strokeColor(rgb(0x99, 0xFF, 0x99));
  ctx.strokeWidth(1.0f / state->camera.scale);

  uint32_t blob_count = sb_count(state->blobs);

  if (state->mouse.was_down && !state->mouse.down) {
    Segment cut = {
      .start = state->camera.pointToWorld(state->mouse.down_pos),
      .end = state->camera.pointToWorld(state->mouse.pos)
    };
    vec2 cut_normal = cut.normal();

    Blob **new_blobs = nullptr;
    printf("brick-engine %u\n", __LINE__);
    for (uint32_t i=0; i<blob_count; i++) {
      Blob *blob = state->blobs[i];
      if (!blob) {
        printf("invalid blob!\n");
        continue;
      }
      {
        Blob **blobs = blob->slice_with_polyline(
          sb_count(state->polyline_slicer),
          state->polyline_slicer
        );

        if (!blobs) {
          sb_push(new_blobs, blob);
          continue;
        }

        delete blob;
        state->blobs[i] = nullptr;

        uint32_t blob_count = sb_count(blobs);
        for (uint32_t i=0; i<blob_count; i++) {
          if (!blobs[i]) {
            continue;
          }

            blobs[i]->circle_pack();
          #if 1
            blobs[i]->circle_graph();
            Blob **islands = blobs[i]->extract_islands();
            uint32_t island_count = sb_count(islands);
            for (uint32_t j=0; j<island_count; j++) {
              if (!islands[j]) {
                continue;
              }
              // TODO: move islands away from each other along the cut normal
              vec2 center = islands[j]->pos;
              float o = cut.orientation(center);
              islands[j]->pos -= (o * cut_normal) * 10.0f;

              sb_push(new_blobs, islands[j]);
            }
            delete blobs[i];
            blobs[i] = nullptr;
          #else
            sb_push(new_blobs, blobs[i]);
          #endif
        }

        sb_free(blobs);
      }
    }

    sb_free(state->blobs);
    state->blobs = new_blobs;
    blob_count = sb_count(state->blobs);
  }

  for (uint32_t i=0; i<blob_count; i++) {
    Blob *blob = state->blobs[i];
    if (!blob) {
      continue;
    }

    if (blob->sdf) {
      blob->sdf->debug_dist();
    } else {
      printf("NULL SDF %u\n", __LINE__);
    }

    blob->render_circles(ctx);
    // blob->render(ctx);

    // draw the previous cut lines
    if (0) {
      uint32_t cut_count = sb_count(state->cuts);
      ctx.strokeColor(rgb(0, 0, 255));
      for (uint32_t i=0; i<cut_count; i++) {

        ctx.beginPath();
          ctx.moveTo(state->cuts[i].start);
          ctx.lineTo(state->cuts[i].end);
          ctx.stroke();
      }
    }

  }

  state->camera.end();

}

