#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
#include <stb_sb.h>
using namespace glm;

#include <string.h>

#include "polygon.h"
#include "context-2d.h"
#include "mouse.h"
#define TAU 6.283185307179586

static char tmp_str[2048] = "\0";

struct State {
  vec3 *particles;

  Mouse mouse;
  Polygon **polygons;
  Polygon *pending_polygon;
};

void setup() {
  auto state = rawkit_hot_state("state", State);

  // build up a floor polygon
  {
    Polygon *polygon = new Polygon("floor");
    polygon->append(vec2(0.0, 0.0));
    polygon->append(vec2((float)rawkit_window_width(), 0.0));
    polygon->append(vec2((float)rawkit_window_width(), 100.0));
    polygon->append(vec2(0.0, 100.0));
    polygon->build_sdf();
    sb_push(state->polygons, polygon);
  }
}

void loop() {
  auto state = rawkit_hot_state("state", State);
  state->mouse.tick();
  Context2D ctx;

  // poly draw handler
  {
    uint32_t c = sb_count(state->polygons);
    // create+append to a poly
    if (state->mouse.down) {
      if (!state->pending_polygon) {
        sprintf(tmp_str, "polygon#%u", c);
        printf("create new polygon: %s\n", tmp_str);
        state->pending_polygon = new Polygon(tmp_str);
      }
      state->pending_polygon->append(state->mouse.pos);
    } else if (state->pending_polygon) {
      uint32_t pc = sb_count(state->pending_polygon->points);
      if (pc < 3) {
        delete state->pending_polygon;
        state->pending_polygon = nullptr;
      } else {
        // rebase the polygon on the origin
        AABB aabb = state->pending_polygon->aabb;
        state->pending_polygon->rebase_on_origin();
        aabb = state->pending_polygon->aabb;

        state->pending_polygon->build_sdf();
        state->pending_polygon->circle_pack();
        sb_push(state->polygons, state->pending_polygon);
        state->pending_polygon = NULL;
      }
    }
  }

  // render polygons
  {
    if (state->pending_polygon) {
      state->pending_polygon->render(ctx);
    }

    uint32_t c = sb_count(state->polygons);
    ctx.fillColor(rgb(0xFF, 0, 0));
    for (uint32_t i=0; i<c; i++) {
        // vec2 nearest = state->polygons[i]->aabb.nearest(state->mouse.pos);
        // igText("nearest(%f, %f)", nearest.x, nearest.y);
        // ctx.beginPath();
        // ctx.arc(
        //   nearest,
        //   2.0
        // );
        // ctx.fill();
      if (state->polygons[i]->aabb.contains(state->mouse.pos)) {
        // igText("mouse over %i (d=%f)",
        //   i,
        //   state->polygons[i]->sdf->sample(state->mouse.pos - state->polygons[i]->aabb.lb)
        // );



        // ctx.strokeColor(rgb(0xFF, 0, 0xFF));
        // for (float x= -0.5; x<=0.5; x+=0.1) {
        //   vec2 off(x, 0);
        //   vec2 normal = state->polygons[i]->sdf->(off + state->mouse.pos - state->polygons[i]->aabb.lb);
        //   normal = normalize(normal);
        //   igText("normal(%f, %f)", normal.x, normal.y);

        //   ctx.beginPath();
        //     ctx.moveTo(state->mouse.pos);
        //     vec2 end = state->mouse.pos + normal * 100.0f;
        //     ctx.lineTo(end);
        //     ctx.stroke();
        // }
      }

      ctx.fillColor(rgb(0xFF, 0xFF, 0xFF));
      state->polygons[i]->render(ctx);
    }
    // if (c>0 && state->polygons[0] && state->polygons[0]->sdf) {
    //   state->polygons[0]->build_sdf();
    //   state->polygons[0]->sdf->debug_dist();
    // }
  }

  // // debug render of texture
  // {
  //   ImTextureID texture = rawkit_imgui_texture(state->bricks[0].tex, state->bricks[0].tex->default_sampler);
  //   if (!texture) {
  //     return;
  //   }

  //   // render the actual image
  //   igImage(
  //     texture,
  //     (ImVec2){ (float)state->bricks[0].tex->options.width, (float)state->bricks[0].tex->options.height },
  //     (ImVec2){ 0.0f, 0.0f }, // uv0
  //     (ImVec2){ 1.0f, 1.0f }, // uv1
  //     (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
  //     (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
  //   );
  // }


}