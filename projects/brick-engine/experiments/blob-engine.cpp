#include <rawkit/rawkit.h>
#include "context-2d.h"
#include "sdf.h"
#include "blob.h"
#include "mouse.h"

struct State {
  Mouse mouse;
  Blob **blobs;
};


void setup () {
  State *state = rawkit_hot_state("state", State);

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


    polygon->build_sdf(blob->sdf);
    delete polygon;

    blob->circle_pack();
    blob->circle_graph();
    blob->extract_islands();
    sb_push(state->blobs, blob);

    // slice
    if (1) {
      Blob **blobs = blob->slice_with_line(vec2(0.0), vec2(10.0, 10.0));

      if (blobs != nullptr) {
        uint32_t blob_count = sb_count(blobs);
        for (uint32_t i=0; i<blob_count; i++) {

          blobs[i]->circle_pack();
          blobs[i]->circle_graph();
          Blob **islands = blobs[i]->extract_islands();
          sb_push(state->blobs, blobs[i]);
          uint32_t island_count = sb_count(islands);
          if (island_count > 1) {
            for (uint32_t j=0; j<island_count; j++) {
              sb_push(state->blobs, islands[j]);
            }
          }
        }
        sb_free(blobs);
      }
    }
  }
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  Context2D ctx;
  ctx.fillColor(rgb(0x99, 0x99, 0x99));
  ctx.strokeColor(rgb(0x99, 0xFF, 0x99));



  uint32_t blob_count = sb_count(state->blobs);
  float scale = 2.5f;
  float inv_scale = 1.0f / scale;
  for (uint32_t i=0; i<blob_count; i++) {
    Blob *blob = state->blobs[i];
    blob->sdf->debug_dist();
    // draw the packed circles
    {

      ctx.save();
        ctx.translate(vec2(0.0, .0f + float(i) * 200.0f));
        ctx.scale(vec2(scale));
        ctx.strokeWidth(inv_scale);


        uint32_t circle_count = sb_count(blob->circles);
        igText("circle count: %u", circle_count);
        for (uint32_t ci=0; ci<circle_count; ci++) {
          PackedCircle circle = blob->circles[ci];

          ctx.beginPath();
            ctx.arc(circle.pos, circle.radius);
            ctx.fill();
        }

        // uint32_t circle_edges_count = sb_count(blob->circle_edges);
        // for (uint32_t cei=0; cei<circle_edges_count; cei++) {
        //   Edge e = blob->circle_edges[cei];
        //   ctx.beginPath();
        //     ctx.moveTo(blob->circles[e.a].pos);
        //     ctx.lineTo(blob->circles[e.b].pos);
        //     ctx.stroke();
        // }
      ctx.restore();
    }

  }

}

