#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <functional>

#include <glm/glm.hpp>
using namespace glm;

#include "prof.h"
#include "state.h"
#include "renderer/vg.h"

void setup() {
  State *state = rawkit_hot_state("state", State);
  state->camera.setup(1.0f, vec2(.01, 10000.0));
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
    return -3;
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
  state->mouse.tick();

  // recalculate the tree on every frame
  float tree_radius = 0.0f;
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
      4600 + sinf(now * 0.25) * 1600.0f,
      200 + cosf(now * 0.5f) * 100.0f,
      0,
      256
    ));

    float sdf_radius = 0.0f;
    {
      u32 c = sb_count(state->ops);
      for (u32 i=0; i<c; i++) {
        vec4 op = state->ops[i];
        sdf_radius = glm::max(
          sdf_radius,
          length(vec3(op)) + op.w
        );
      }
    }

    const vec3 sdf_center(0.0);
    tree_radius = next_power_of_two(sdf_radius);
    fillInner(state, tree_radius, sdf_center, sample_scene, 0);
  }

  // visualize
  {
    igText("tree_radius: %f dense voxels: %0.1fB", tree_radius, glm::pow(tree_radius, 3.0) / glm::pow(1000, 3));
    igText("nodes inner(%llu) leaf(%llu)", sb_count(state->nodes), sb_count(state->leaves));
    renderer_vg(state, tree_radius);
  }
}