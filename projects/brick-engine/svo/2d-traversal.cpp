#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
using namespace glm;

#include "../experiments/camera-2d.h"
#include "../experiments/context-2d.h"
#include "../experiments/mouse.h"

struct State {
  Mouse mouse;
  Camera2D *camera;
  float t;
};


void setup() {
  State *state = rawkit_hot_state("state", State);
  if (!state->camera) {
    state->camera = new Camera2D();
  }

}

struct Box {
  vec2 lb;
  vec2 ub;
  void render(Context2D &ctx, u32 color = 0xFFFFFFFF) {
    vec2 mid = (this->lb + this->ub) * 0.5f;
    ctx.beginPath();
      ctx.rect(this->lb, this->ub - this->lb);
      ctx.moveTo(this->lb.x, mid.y);
      ctx.lineTo(this->ub.x, mid.y);
      ctx.moveTo(mid.x, this->lb.y);
      ctx.lineTo(mid.x, this->ub.y);
      ctx.strokeColor(rgb(color));
      ctx.stroke();
  }

  vec2 isect_ray(vec2 origin, vec2 dir) {
    vec2 inv_dir = 1.0f / dir;

    vec2 tbot = inv_dir * (this->lb - origin);
    vec2 ttop = inv_dir * (this->ub - origin);
    vec2 tmin = min(ttop, tbot);
    vec2 tmax = max(ttop, tbot);
    float t0 = max(tmin.x, tmin.y);
    float t1 = min(tmax.x, tmax.y);

    if (t1 <= max(t0, 0.0f)) {
      return vec2(-1.0f);
    }

    return vec2(t0, t1);
  }

  vec2 center() {
    return (this->lb + this->ub) * 0.5f;
  }

  u8 quadrant(vec2 p) {
    #if 1
    vec2 c = (this->lb + this->ub) * 0.5f;
    float r = distance(c, this->ub);

    bvec2 v = greaterThanEqual(((p - c) / r) + 1.0f, vec2(1.0));
    #else
    bvec2 v = greaterThanEqual(
      glm::clamp((p - this->center()), vec2(0.0), vec2(2.0)),
      vec2(1.0)
    );
    #endif
    return (v.x << 0 | v.y << 1);

  }

  Box quadrantToBox(u8 v) {
    vec2 c = (this->lb + this->ub) * 0.5f;
    float r = (ub.x - c.x) * 0.5f;
    vec2 bc = c + vec2(
      (v & 1) > 0 ? r : -r,
      (v & 2) > 0 ? r : -r
    );
    return {
      .lb = bc - r,
      .ub = bc + r
    };
  }
};

float copysign(float a, float b) {
  return b >= 0.0 ? a : -a;
}

float max2(vec2 a) {
  return glm::max(a.x, a.y);
}

float min2(vec2 a) {
  return glm::min(a.x, a.y);
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  Context2D ctx;
  state->mouse.tick();
  state->camera->ctx = ctx;
  state->camera->tick(
    state->mouse.button(ImGuiMouseButton_Left),
    state->mouse.pos,
    state->mouse.wheel * 0.1f
  );

  float radius = 0.5f;
  Box box = {
    .lb = vec2(0.0),
    .ub = vec2(2.0)
  };

  state->camera->begin();
    ctx.scale(10.0, -10.0);
    ctx.strokeWidth(0.25 / (state->camera->scale * 10.0));
    box.render(ctx);

    if (state->mouse.button(ImGuiMouseButton_Right)) {
      state->t += 0.01;
    }

    vec2 ro(
      cos(state->t) * 2.0 + 1.0,
      sin(state->t) * 2.0 + 1.0
    );
    // vec2 rd(1.0, glm::abs(sin(rawkit_now()) * 2.0));

    // vec2 rd = normalize(
    //   state->camera->pointToWorld(
    //     vec2(
    //       state->mouse.pos.x,
    //       (float)rawkit_window_height() - state->mouse.pos.y
    //     )
    //   ) - ro
    // );
    vec2 rd = normalize(1.0f - ro);//vec2(1.0, 0.75 + sin(rawkit_now())));
    // rd = normalize(vec2(1.1, sin(rawkit_now() * 0.1)) - ro);
    vec2 ard = abs(rd);
    vec2 aro = ro;
    vec2 ird = 1.0f / ard;

    u8 quadrant_mask = 0;
    if (rd.x < 0.0) {
      quadrant_mask ^= 1;
      ard.x = -rd.x;
      // flip the origin over the midpoint (1, 1)
      if (ro.x > 1.0) {
        aro.x = 2.0 - ro.x;
      } else {
        aro.x = 1.0 - ro.x;
      }
    }

    if (rd.y < 0.0) {
      quadrant_mask ^= 2;
      ard.y = -rd.y;
      // flip the origin over the midpoint (1, 1)
      if (ro.y > 1.0) {
        aro.y = 2.0 - ro.y;
      } else {
        aro.y = 1.0 - ro.y;
      }
    }

    ard = normalize(ard);
    vec2 aird = 1.0f / ard;

    vec2 r = box.isect_ray(aro, ard);
    vec2 start = aro + ard * r.x;
    vec2 end = aro + ard * r.y;

    {
      vec2 inv_dir = 1.0f / rd;

      ctx.beginPath();
        ctx.moveTo(ro);
        ctx.lineTo(ro + rd * 1000.0f);
        ctx.strokeColor(rgb(0x00FF00FF));
        ctx.strokeWidth(0.5 / (state->camera->scale * 10.0));
        ctx.stroke();

      // render sign normalized ray
      // ctx.beginPath();
      //   ctx.moveTo(aro);
      //   ctx.lineTo(aro + ard * 1000.0f);
      //   ctx.strokeColor(rgb(0x0000FFFF));
      //   ctx.strokeWidth(0.5 / (state->camera->scale * 10.0));
      //   ctx.stroke();
    }


  // iterate
  #if 1
    float epsilon = glm::pow(2.0f, -32.0f);
    igText("r(%f, %f)", r.x, r.y);
    if (r.x >= r.y) {
      igText("r.x >= r.y");
      return;
    }

    u32 colors[] = { 0xFF0000FF, 0x00FF00FF, 0x0000FFFF };
    u8 quadrant = box.quadrant(start);

    // t.x(x) = ird * x + (-p.x / d.x)
    // t.y(y) = ird * y + (-p.y / d.y)
    vec2 coef = -start / ard;

    igText("start(%f, %f)", start.x, start.y);
    igText("ard(%f, %f)", ard.x, ard.y);

    for (u32 i=0; i<3; i++) {
      Box child = box.quadrantToBox(quadrant ^ quadrant_mask);
      child.render(ctx, colors[i]);

      // TODO: this needs to be the current scale as we recurse
      vec2 upperCorner(
        (quadrant & 1<<0) ? 2.0 : 1.0,
        (quadrant & 1<<1) ? 2.0 : 1.0
      );

      vec2 crossings = aird * upperCorner + coef;
      u8 next_quadrant = quadrant | (crossings.x <= crossings.y ? 1 : 2);
      if (next_quadrant <= quadrant) {
        break;
      }
      quadrant = next_quadrant;
    }
  #else
    Box firstBox = box.quadrantToBox(box.quadrant(start));
    {
      firstBox.render(ctx, 0xFF0000FF);
    }

    if (steps > 0) {
      Box lastBox = box.quadrantToBox(box.quadrant(end));
      lastBox.render(ctx, 0x00FF00FF);
    }

    if (steps > 1) {
      #if 0
        // brute force computation of the middle quadrant
        vec2 r1 = firstBox.isect_ray(ro, rd);
        vec2 r2 = lastBox.isect_ray(ro, rd);
        vec2 out1 = ro + rd * r1.y;
        vec2 out2 = ro + rd * r2.x;
        Box middleBox = box.quadrantToBox(box.quadrant((out1 + out2) * 0.5f));
      #else
        // "enlightened" middle node calculation
        vec2 exits = inv_dir * (box.center() - ro);
        Box middleBox = box.quadrantToBox(exits.x <= exits.y ? 1 : 2);
      #endif
      middleBox.render(ctx, 0x0000FFFF);
    }
  #endif
  state->camera->end();
}