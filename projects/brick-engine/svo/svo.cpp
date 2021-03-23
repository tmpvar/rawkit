#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <functional>

#include <glm/glm.hpp>
using namespace glm;

#define CPU_HOST
#include "svo.h"
#include "../experiments/context-2d.h"
#include "../experiments/camera-2d.h"
#include "../experiments/mouse.h"

struct State {
  Camera2D camera;
  Mouse mouse;
  InnerNode *nodes;
  LeafNode *leaves;
  vec4 *node_positions;
  vec4 *ops;
};

struct Prof {
  const char *name;
  double start = 0.0;
  bool finished = false;
  Prof(const char *name)
  : name(name)
  {
  this->start = rawkit_now();
  }

  double diff_ms() {
  return (rawkit_now() - this->start) * 1000.0;
  }

  ~Prof() {
  this->finish();
  }

  void finish() {
  if (this->finished) {
    return;
  }
  this->finished = true;
  igText("%s took %f ms", this->name, diff_ms());
  }
};

void setup() {
  State *state = rawkit_hot_state("state", State);
  state->camera.setup();
}

typedef std::function<float(vec3 pos)> sample_fn_t;


u32 next_power_of_two(u32 n) {
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}

float sample_scene(vec3 pos) {
  State *state = rawkit_hot_state("state", State);
  float d = FLT_MAX;
  u32 c = sb_count(state->ops);
  for (u32 i=0; i<c; i++) {
    vec4 op = state->ops[i];
    d = glm::min(
      d,
      distance(pos, vec3(op)) - op.w
    );
  }

  return d;
}


void fillLeaf(State *state, const InnerNode *parent, const vec3 parent_pos, sample_fn_t sample) {



}

i32 fillInner(State *state, const float radius, const vec3 center, sample_fn_t sample, u32 depth) {
  const u32 max_depth = 10;
  if (depth > max_depth) {
    return -1;
  }
  float child_radius = radius * 0.5f;
  // TODO: if the children will be leaves then fillLeaf
  if (radius < 4.0f) {
    return -2;
  }

  float dist = sample_scene(center);
  if (abs(dist) >= glm::length(vec3(radius))) {
    // printf("outside sdf\n");
    return -3;
  }

  if (depth > max_depth-1) {
    State *state = rawkit_hot_state("state", State);
    Context2D ctx = state->camera.ctx;
    // ctx.fillColor(hsl((float)depth/(float)max_depth, 0.9, 0.6));

    if (dist <= radius)  {
      ctx.fillColor(rgb(0xaaaaaaff));
      // ctx.fillColor(hsl(abs(dist), 0.9, 0.6));
      ctx.beginPath();
        ctx.rect(vec2(center) - radius, vec2(radius * 2.0));
        ctx.fill();

      // igText("here (%f, %f, %f) = %f", center.x, center.y, center.z, abs(dist) - radius);
      ctx.beginPath();
        ctx.fillColor(rgb(0x000000ff));
        ctx.arc(center, 1.0);
        ctx.fill();
    }
  }
  sb_push(state->node_positions, vec4(center, radius));

  InnerNode node = {};

  // test the octants and recurse as necessary
  bool valid = false;
  for (u8 i = 0; i<8; i++) {
    float x = (i&1<<0) == 0 ? -child_radius : child_radius;
    float y = (i&1<<1) == 0 ? -child_radius : child_radius;
    float z = (i&1<<2) == 0 ? -child_radius : child_radius;
    vec3 p = center + vec3(x, y, z);
    i32 r = fillInner(state, child_radius, p, sample, depth+1);

    if (r > -1) {
      valid = true;
    }

    node.children[i] = r;
  }

  // if (!valid) {
  //   return -1;
  // }

  i32 c = sb_count(state->nodes);
  sb_push(state->nodes, node);

  return c;
}

void loop() {
  Prof loop_prof("loop");
  State *state = rawkit_hot_state("state", State);

  Context2D ctx;
  state->camera.ctx = ctx;
  state->mouse.tick();
  state->camera.tick(
    igIsMouseDown(ImGuiMouseButton_Middle),
    state->mouse.pos,
    state->mouse.wheel * 0.1f
  );
  state->camera.begin();
  // recalculate the tree on every frame
  {
    Prof rebuild_prof("rebuild");
    sb_reset(state->leaves);
    sb_reset(state->nodes);
    sb_reset(state->node_positions);
    sb_reset(state->ops);

    // TODO: compute the bounds
    sb_push(state->ops, vec4(0, 0, 0, 128));

    float now = rawkit_now();

    sb_push(state->ops, vec4(
      sinf(now) * 1200.0f,
      cosf(now) * 1200.0f,
      0,
      90
    ));

    sb_push(state->ops, vec4(
      200 + sinf(now) * 100.0f,
      200 + cosf(now * 0.5f) * 100.0f,
      0,
      90
    ));

    sb_push(state->ops, vec4(
      600 + sinf(now * 0.25) * 600.0f,
      200 + cosf(now * 0.5f) * 100.0f,
      0,
      190
    ));

    const float sdf_radius = 4096.0f;
    const vec3 sdf_center(0.0);
    const float tree_radius = next_power_of_two(sdf_radius);
igText("tree_radius: %f", tree_radius);
    ctx.translate(vec2(800));
    fillInner(state, tree_radius, sdf_center, sample_scene, 0);

    igText("nodes inner(%llu) leaf(%llu)", sb_count(state->nodes), sb_count(state->leaves));

    // debug
    igText("camera translation(%f, %f)", state->camera.translation.x, state->camera.translation.y);


    ctx.strokeWidth(1);
    if (0) {
      u32 c = sb_count(state->node_positions);
      // c = glm::min(c, u32(5000));
      ctx.strokeColor(rgb(0xFFFFFFFF));
      for (u32 i=0; i<c; i++) {
        vec4 pos = state->node_positions[i];
        float r = pos.w;
        if (pos.w < 16) {
          continue;
        }
        ctx.beginPath();
          ctx.rect(vec2(pos) - r, vec2(r));
        //   ctx.moveTo(vec2(pos) - r);
        //   ctx.lineTo(vec2(pos) + r);
        //   ctx.moveTo(vec2(pos.x - r, pos.y + r));
        //   ctx.lineTo(vec2(pos.x + r, pos.y - r));

        ctx.strokeColor(hsl(pos.w / tree_radius, 0.9, 0.6));
        ctx.stroke();
      }
    }


    {
      u32 c = sb_count(state->ops);
      for (u32 i=0; i<c; i++) {
        vec4 op = state->ops[i];
        ctx.beginPath();
          ctx.arc(vec3(op), op.w);
          ctx.strokeColor(rgb(0xFFFFFFFF));
          ctx.stroke();
      }
    }
    state->camera.end();
  }



}