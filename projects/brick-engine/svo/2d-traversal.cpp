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
  float radius;
  Box() {}
  Box(vec2 lb, vec2 ub) {
    this->lb = lb;
    this->ub = ub;
    this->c_center = (lb + ub) * 0.5f;
    this->radius = ub.x - c_center.x;
  }

  Box(vec2 center, float radius) {
    this->lb = center - radius;
    this->ub = center + radius;
    this->c_center = center;
    this-> radius = radius;
  }

  void render(Context2D &ctx, u32 color = 0xFFFFFFFF) {
    this->render(ctx, rgb(color));
  }

  void render(Context2D &ctx, Color color) {
    vec2 mid = (this->lb + this->ub) * 0.5f;
    ctx.beginPath();
      ctx.rect(this->lb, this->ub - this->lb);
      // ctx.moveTo(this->lb.x, mid.y);
      // ctx.lineTo(this->ub.x, mid.y);
      // ctx.moveTo(mid.x, this->lb.y);
      // ctx.lineTo(mid.x, this->ub.y);
      ctx.strokeColor(color);
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
    bvec2 v = greaterThanEqual(
      (p - c_center) / this->radius,
      vec2(0.0)
    );
    #else
    bvec2 v = greaterThanEqual(
      glm::clamp((p - this->center()), vec2(0.0), vec2(2.0)),
      vec2(1.0)
    );
    #endif
    return (v.x << 0 | v.y << 1);

  }

  Box quadrantToBox(u8 v) {
    float r = (ub.x - c_center.x) * 0.5f;
    vec2 bc = c_center + vec2(
      (v & 1) > 0 ? r : -r,
      (v & 2) > 0 ? r : -r
    );
    return Box(bc, r);
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
    state->mouse.wheel
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

    // The original paper converts all ray directions to negative, but I'll stick with
    // positive for now.
    u8 quadrant_mask = 0;
    if (rd.x < 0.0) {
      igText("invert ray x");
      quadrant_mask |= 1;
      ard.x = -rd.x;
      aro.x = -ro.x;
    }

    if (rd.y < 0.0) {
      igText("invert ray y");
      quadrant_mask |= 2;
      ard.y = -rd.y;
      aro.y = -ro.y;
    }
    ard = normalize(ard);
    vec2 root_isect = box.isect_ray(ro, rd);

    vec2 aird = 1.0f / ard;



    vec2 start = aro + ard * root_isect.x;
    // vec2 end = aro + ard * root_isect.y;

    // setup the ray coefficient:
    //   given the upper bounds of a node return the x/y crossing
    //   which can be used
    // t(upper_corner) = abs_inv_rd * upper_corner + (-p.x / d.x)
    vec2 coef = -aro / ard;
    vec2 bias = aro * coef;


    igText("rd(%f, %f)", rd.x, rd.y);
    igText("ro(%f, %f)", ro.x, ro.y);
    igText("coef(%f, %f)", coef.x, coef.y);
    igText("bias(%f, %f)", bias.x, bias.y);
    igText("t(%f, %f)",
      max(0.0f, min2(coef - bias)),
      min(1.0f, max2(2.0f * coef - bias))
    );
    igText("corner(%f, %f)",
      -bias.x,
      -bias.y
    );


    igText("ard(%f, %f) aird(%f, %f)", ard.x, ard.y, aird.x, aird.y);
    igText("isect(%f, %f)", root_isect.x, root_isect.y);
    vec2 root_interval = root_isect;//aird * box.ub + coef;

    // render the ray
    {
      ctx.beginPath();
      ctx.moveTo(ro);
      ctx.lineTo(ro + rd * 1000.0f);
      ctx.strokeColor(rgb(0x00FF00FF));
      ctx.stroke();

      // ctx.beginPath();
      // ctx.moveTo(aro);
      // ctx.lineTo(aro + ard * 1000.0f);
      // ctx.strokeColor(rgb(0xFF0000FF));
      // ctx.stroke();

    }

    // iterate
    u32 colors[] = { 0xFF0000FF, 0x00FF00FF, 0x0000FFFF };

    // define the upper bounds of the lower and upper quadrants
    vec2 corners(0.0, radius);

    struct StackEntry {
      Box box;
      u8 quadrant;
      // .x = tmin, .y = tmax
      vec2 t_interval;
    };

    #define MAX_ENTRIES 10
    struct Stack {
      StackEntry entries[MAX_ENTRIES];
      i32 idx = 0;

      void push(const Box box, u8 quadrant, vec2 interval) {
        if (idx >= 31) {
          printf("overflow!\n");
          return;
        }
        this->entries[idx++] = {
          .box = box,
          .quadrant = quadrant,
          .t_interval = interval,
        };
      }

      void pop() {
        idx--;
      }

      StackEntry *last() {
        if (idx < 1) {
          return nullptr;
        }
        return &this->entries[idx-1];
      }

      i32 length() {
        return idx;
      }
    };

    Stack stack;
    stack.push(box, box.quadrant(start), root_interval);

    // igText("start(%f, %f)", start.x, start.y);
    igText("box.center(%f, %f)", box.c_center.x, box.c_center.y);
    igText("box.quadrant(aro) = %u", box.quadrant(aro));


    i32 s = 400;
    float t = root_interval.x;
    while(stack.length() && s--) {
      StackEntry *parent = stack.last();

      // igText("max_t(%f) t(%f)  parent_t(%f, %f)", root_interval.y, t, parent->t_interval.x, parent->t_interval.y);
      parent->box.quadrantToBox(parent->quadrant).render(
        ctx,
        hsl(float(stack.length())/float(MAX_ENTRIES), 0.9, 0.6)
      );

      // Pop
      if (1) {
        if (t >= parent->t_interval.y) {
          igText("POP - t past parent t_max");

          stack.pop();
          continue;
        }
      }

      // Push
      if (1) {
        if (t < parent->t_interval.y && stack.length() < 5) {
          vec2 pos = ro + rd * t;
          u8 child_quadrant = parent->box.quadrant(pos);
          igText("PSH level(%u) quadrant(%u) t(%f) pos(%f, %f)", stack.length(), child_quadrant, t, pos.x, pos.y);
          igText("  parent.t_interval(%f, %f)", parent->t_interval.x, parent->t_interval.y);
          Box child = parent->box.quadrantToBox(child_quadrant);
          igText("  parent[(%0.4f, %0.4f) - (%0.4f, %0.4f), radius(%0.4f)]\n  child[(%0.4f, %0.4f) - (%0.4f, %0.4f), radius(%0.4f)]",
            parent->box.lb.x,
            parent->box.lb.y,
            parent->box.ub.x,
            parent->box.ub.y,
            parent->box.radius,
            child.lb.x,
            child.lb.y,
            child.ub.x,
            child.ub.y,
            child.radius
          );

          vec2 crossings = aird * abs(child.ub) + coef;
          igText("  quadrant(%u) crossings(%f, %f) ",child.quadrant(pos), crossings.x, crossings.y);

          stack.push(
            child,
            child.quadrant(pos),
            child.isect_ray(ro, rd)
          );
          parent->box.quadrantToBox(child_quadrant).render(
            ctx,
            hsl(float(stack.length())/float(MAX_ENTRIES), 0.9, 0.6)
          );
          continue;
        }
      }

      // Advance
      {
        vec2 upperCorner(
          (parent->quadrant & 1<<0) == 0 ? parent->box.c_center.x : parent->box.ub.x,
          (parent->quadrant & 1<<1) == 0 ? parent->box.c_center.y : parent->box.ub.y
        );

        vec2 crossings = aird * upperCorner + coef;
        u8 next_quadrant = parent->quadrant | (crossings.x <= crossings.y ? 1 : 2);
        float old_t = t;
        if (next_quadrant <= parent->quadrant) {
          stack.pop();
          t = parent->t_interval.y + 0.001;
          igText("ADV level(%u) quadrant(%u) next_quadrant(%u) t(%f -> %f)", stack.length(), parent->quadrant, next_quadrant, old_t, t);
          igText("  POP t(%f -> %f)", parent->quadrant, old_t, t);


          continue;
        } else {
          t = parent->box.quadrantToBox(next_quadrant).isect_ray(ro, rd).x;
          igText("ADV level(%u) quadrant(%u) next_quadrant(%u) t(%f -> %f)", stack.length(), parent->quadrant, next_quadrant, old_t, t);
        }


        // parent->box.quadrantToBox(next_quadrant ^ quadrant_mask).render(
        //   ctx,
        //   hsl(float(stack.length())/float(MAX_ENTRIES), 0.9, 0.6)
        // );

        parent->quadrant = next_quadrant;
      }
    }
  state->camera->end();
}