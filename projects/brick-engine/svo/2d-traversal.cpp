#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
using namespace glm;

#include "../experiments/camera-2d.h"
#include "../experiments/context-2d.h"
#include "../experiments/mouse.h"

struct State {
  Mouse mouse;
  Camera2D *camera;
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
    igIsMouseDown(ImGuiMouseButton_Left),
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

    vec2 ro(
      cos(rawkit_now()) * 2.0 + 1.0,
      sin(rawkit_now()) * 2.0 + 1.0
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
    vec2 ird = 1.1f / rd;

    u8 octant_mask = (
      ((rd.x < 0.0) ? 1 : 0) |
      ((rd.y < 0.0) ? 2 : 0)
    );

    vec2 r = box.isect_ray(ro, rd);
    vec2 start = ro + rd * r.x;
    vec2 end = ro + rd * r.y;

    {
      vec2 inv_dir = 1.0f / rd;

      ctx.beginPath();
        ctx.moveTo(ro);
        ctx.lineTo(ro + rd * 1000.0f);
        ctx.strokeColor(rgb(0x00FF00FF));
        ctx.strokeWidth(0.5 / (state->camera->scale * 10.0));
        ctx.stroke();

      vec2 center = (start + end) * 0.5f;
      ctx.beginPath();
        ctx.arc(start, 1.0 / (state->camera->scale * 10.0));
        ctx.fillColor(rgb(0xFF0000FF));
        ctx.fill();

      ctx.beginPath();
        ctx.arc(end, 1.0 / (state->camera->scale * 10.0));
        ctx.fillColor(rgb(0x0000FFFF));
        ctx.fill();

      u8 diff = box.quadrant(start) ^ box.quadrant(end);
      u8 steps = ((diff & 1) ? 1 : 0) + ((diff & 2) ? 1 : 0);
      igText("quadrants: start(%u) end(%u) diff(%u) steps(%u)",
        box.quadrant(start),
        box.quadrant(end),
        diff,
        steps
      );

      igText("ray_dir(%f, %f) slope(%f)", rd.x, rd.y, rd.x/rd.y);

      {
        vec2 c = (box.lb + box.ub) * 0.5f;
        float r = distance(c, box.ub);
        vec2 hrm = vec2((ro - c) / r) + 1.0f;
        igText("hrm(%f, %f)", hrm.x, hrm.y);
      }
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
    vec2 coef = -start / rd;

    igText("start(%f, %f)", start.x, start.y);

    for (u32 i=0; i<3; i++) {
      igText("quadrant(%u)", quadrant);
      Box child = box.quadrantToBox(quadrant);
      child.render(ctx, colors[i]);

      vec2 upperCorner(
        (quadrant & 1<<0) ? 2.0 : 1.0,
        (quadrant & 1<<1) ? 2.0 : 1.0
      );
      vec2 crossings = ird * upperCorner + coef;
      u8 next_quadrant = quadrant ^ (crossings.x < crossings.y ? 1 : 2);
      if (next_quadrant < quadrant) {
        igText("OOB");
        break;
      }
      quadrant = next_quadrant;
      igText("crossings(%f, %f)", crossings.x, crossings.y);
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