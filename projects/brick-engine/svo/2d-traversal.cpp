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
  vec2 c_center;
  Box() {}
  Box(vec2 lb, vec2 ub) {
    this->lb = lb;
    this->ub = ub;
    this->c_center = (lb + ub) * 0.5f;
  }

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
    return this->c_center;
  }

  u8 quadrant(vec2 p) {
    #if 1
    float r = distance(c_center, this->ub);

    bvec2 v = greaterThanEqual(((p - c_center) / r) + 1.0f, vec2(1.0));
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
    return Box(bc - r, bc + r);
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

  float radius = 1.0f;
  Box box(vec2(-1.0), vec2(1.0));

  state->camera->begin();
    ctx.scale(50.0, -50.0);
    ctx.strokeWidth(0.75 / (state->camera->scale * 50.0));
    box.render(ctx);

    if (state->mouse.button(ImGuiMouseButton_Right)) {
      state->t += 0.01;
    }

    vec2 ro = box.center() + vec2(
      cos(state->t) * 2.0,
      sin(state->t) * 2.0
    );
    // vec2 rd = normalize(
    //   state->camera->pointToWorld(
    //     vec2(
    //       state->mouse.pos.x,
    //       (float)rawkit_window_height() - state->mouse.pos.y
    //     )
    //   ) - ro
    // );
    vec2 rd = normalize(box.center() - ro);//vec2(1.0, 0.75 + sin(rawkit_now())));

    vec2 ard = abs(rd);
    vec2 aro = ro;
    vec2 ird = 1.0f / ard;

    // The original paper converts all ray directions to negative, but I'll stick with
    // positive for now.
    u8 quadrant_mask = 0;
    if (rd.x < 0.0) {
      quadrant_mask ^= 1;
      ard.x = -rd.x;
      aro.x = -ro.x;
    }

    if (rd.y < 0.0) {
      quadrant_mask ^= 2;
      ard.y = -rd.y;
      aro.y = -ro.y;
    }

    vec2 aird = 1.0f / ard;

    vec2 r = box.isect_ray(aro, ard);
    vec2 start = aro + ard * r.x;
    vec2 end = aro + ard * r.y;

    // render the ray
    {
      ctx.beginPath();
      ctx.moveTo(ro);
      ctx.lineTo(ro + rd * 1000.0f);
      ctx.strokeColor(rgb(0x00FF00FF));
      ctx.stroke();
    }

    // iterate
    u32 colors[] = { 0xFF0000FF, 0x00FF00FF, 0x0000FFFF };
    u8 quadrant = box.quadrant(start);

    // setup the ray coefficient:
    //   given the upper bounds of a node return the x/y crossing
    //   which can be used
    // t(upper_corner) = abs_inv_rd * upper_corner + (-p.x / d.x)
    vec2 coef = -start / ard;

    // define the upper bounds of the lower and upper quadrants
    vec2 corners(0.0, radius);

    for (u32 i=0; i<3; i++) {
      box.quadrantToBox(quadrant ^ quadrant_mask).render(ctx, colors[i]);

      vec2 upperCorner(
        (quadrant & 1<<0) == 0 ? corners.x : corners.y,
        (quadrant & 1<<1) == 0 ? corners.x : corners.y
      );

      vec2 crossings = aird * upperCorner + coef;
      u8 next_quadrant = quadrant | (crossings.x <= crossings.y ? 1 : 2);
      if (next_quadrant <= quadrant) {
        break;
      }
      quadrant = next_quadrant;
    }
  state->camera->end();
}