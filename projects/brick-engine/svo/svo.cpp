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

void fillLeaf(State *state, const InnerNode *parent, const vec3 parent_pos, sample_fn_t sample) {



}

i32 fillInner(State *state, const float radius, const vec3 center, sample_fn_t sample) {
  float child_radius = radius * 0.5f;
  // TODO: if the children will be leaves then fillLeaf
  if (radius < 4.0f) {
    return -2;
  }

  float dist = sample(center);

  if (dist > (radius * 1.7f)) {
    // printf("outside sdf\n");
    return -3;
  }

  InnerNode node = {};

  // test the octants and recurse as necessary
  bool valid = false;
  for (u8 i = 0; i<8; i++) {
    node.children[i] = 0;
    float z = child_radius * ((i<4) ? -1.0f : 1.0f);
    float x = (i&1<<0) == 0 ? -child_radius : child_radius;
    float y = (i&1<<1) == 0 ? -child_radius : child_radius;

    i32 r = fillInner(state, child_radius, center + vec3(x, y, z), sample);

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

  sb_push(state->node_positions, vec4(center, radius));
  return c;
}


void loop() {
  Prof("loop");
  State *state = rawkit_hot_state("state", State);
  Context2D ctx;
  state->mouse.tick();
  state->camera.tick(
    igIsMouseDown(ImGuiMouseButton_Middle),
    state->mouse.pos,
    state->mouse.wheel * 0.1f
  );

  // recalculate the tree on every frame
  {
    Prof("rebuild");
    sb_reset(state->leaves);
    sb_reset(state->nodes);

    const float sdf_radius = 100.0f;
    const vec3 sdf_center(0.0);
    const float tree_radius = next_power_of_two(sdf_radius);

    const vec4 ops[2] = {
      vec4(0, 0, 0, 100),
      vec4(50, 0, 0, 30),
    };

    fillInner(state, tree_radius, sdf_center, [&ops](vec3 pos) -> float {
      // float d = FLT_MAX;
      // for (u32 i=0; i<1; i) {
      //   vec4 op = ops[i];
      //   d = glm::min(d, distance(pos, vec3(op)) - op.w);
      // }
      // return d;
      return 1.0;
    });

    igText("nodes inner(%llu) leaf(%llu)", sb_count(state->nodes), sb_count(state->leaves));

    // u32 c = sb_count(state->nodes);
    // for (u32 i=0; i<glm::min(c, u32(100)); i++) {
    //   InnerNode n = state->nodes[i];
    //   igText("#%llu (%i, %i, %i, %i, %i, %i, %i)",
    //     i,
    //     n.children[0],
    //     n.children[1],
    //     n.children[2],
    //     n.children[3],
    //     n.children[5],
    //     n.children[6],
    //     n.children[7]
    //   );
    // }


    // debug
    igText("camera translation(%f, %f)", state->camera.translation.x, state->camera.translation.y);
    // ctx.translate(vec2(500));
    state->camera.begin();

    ctx.strokeWidth(1);
    u32 c = sb_count(state->node_positions);
    // c = glm::min(c, u32(5000));
    ctx.strokeColor(rgb(0xFFFFFFFF));
    for (u32 i=0; i<c; i++) {
      vec4 pos = state->node_positions[i];
      float r = pos.w;
      if (pos.w < 8) {
        continue;
      }
      ctx.beginPath();
        ctx.rect(vec2(pos) - (r * 0.5f), vec2(r));
        // ctx.moveTo(vec2(pos) - r);
        // ctx.lineTo(vec2(pos) + r);
        // ctx.moveTo(vec2(pos.x - r, pos.y + r));
        // ctx.lineTo(vec2(pos.x + r, pos.y - r));
      ctx.strokeColor(hsl(pos.w / tree_radius, 0.9, 0.6));
      ctx.stroke();
    }

    state->camera.end();
  }



}