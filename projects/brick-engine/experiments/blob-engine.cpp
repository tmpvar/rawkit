#include <rawkit/rawkit.h>
#include "context-2d.h"
#include "camera-2d.h"
#include "sdf.h"
#include "blob.h"
#include "mouse.h"

struct Segment {
  vec2 start;
  vec2 end;
  vec2 normal() {
    vec2 n = normalize(end - start);
    return vec2(-n.y, n.x);
  }

  float orientation(vec2 p) {
    return glm::sign(
      (this->end.y - this->start.y) *
      (p.x         - this->end.x)   -
      (this->end.x - this->start.x) *
      (p.y         - this->end.y)
    );
  }
};

struct State {
  Mouse mouse;
  Blob **blobs;
  Camera2D camera;
  Segment *cuts;
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

    blob->circle_pack();
    blob->circle_graph();
    blob->extract_islands();
    sb_push(state->blobs, blob);

    // slice
    if (0) {
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
          delete blobs[i];
        }

        sb_free(blobs);
      }
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
    ctx.strokeColor(rgb(0xFFFFFFFF));
    ctx.beginPath();
      ctx.moveTo(state->camera.pointToWorld(state->mouse.down_pos));
      ctx.lineTo(state->camera.pointToWorld(state->mouse.pos));
      ctx.stroke();
  }

  ctx.fillColor(rgb(0x99, 0x99, 0x99));
  ctx.strokeColor(rgb(0x99, 0xFF, 0x99));

  uint32_t blob_count = sb_count(state->blobs);

  if (state->mouse.was_down && !state->mouse.down) {
    Segment cut = {
      .start = state->camera.pointToWorld(state->mouse.down_pos),
      .end = state->camera.pointToWorld(state->mouse.pos)
    };
    sb_push(state->cuts, cut);

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
        Blob **blobs = blob->slice_with_line(
          cut.start,
          cut.end
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

              vec2 center = islands[j]->pos + islands[j]->dims * 0.5f;
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

    // draw the packed circles
    {
      ctx.save();
        ctx.translate(blob->pos);
        ctx.strokeWidth(1.0f / state->camera.scale);


        uint32_t circle_count = sb_count(blob->circles);
        igText("  circle count: %u", circle_count);
        igText("  pos: %f, %f", blob->pos.x, blob->pos.y);
        ctx.strokeColor(rgb(0xFF, 0xFF, 0xFF));
        for (uint32_t ci=0; ci<circle_count; ci++) {
          PackedCircle circle = blob->circles[ci];

          ctx.beginPath();
            ctx.arc(circle.pos, circle.radius);
            ctx.stroke();
        }

        // aabb debugging
        if (0) {
          ctx.fillColor(rgb(0xFF, 0, 0));
          ctx.beginPath();
            ctx.arc(vec2(0.0), 1.0f);
            ctx.fill();

          ctx.strokeColor(rgb(0x00, 0xFF, 0x00));
          ctx.beginPath();
            ctx.moveTo(vec2(0.0));
            ctx.lineTo(vec2(blob->dims.x, 0.0));
            ctx.lineTo(blob->dims);
            ctx.lineTo(vec2(0.0, blob->dims.y));
            ctx.lineTo(vec2(0.0));
            ctx.stroke();
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

