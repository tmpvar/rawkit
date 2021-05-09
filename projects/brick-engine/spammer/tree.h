#include "FastNoiseLite.h"

#include <vector>
#include <random>
#include <queue>
using namespace std;

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/random.hpp>
using namespace glm;

std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
auto next_random = std::bind(distribution, generator);

#define MAX_NODE_CHILDREN 16

struct Node {
  vec3 pos;
  vec3 display_pos;
  i32 child_count = 0;
  i32 children[MAX_NODE_CHILDREN];
  i32 parent = -1;
  f32 size = 0;
  i32 depth = 0;
};

struct Edge {
  i32 start;
  i32 end;
};

const u32 max_branches_per_node = 4;
struct Tree {
  vector<Node> verts;
  vector<Edge> edges;

  f32 grow_tick = 0.0f;

  Tree(const vec3 &seed_pos) {
    verts.push_back({
      .pos = vec3(0),
      .display_pos = seed_pos,
      .child_count = 0,
      .parent = -1,
      .size = 1.0f,
      .depth = 0,
    });

    grow_tick = next_random();
  }

  void branch(i32 parent_id, vec3 pos) {
    if (parent_id < 0) {
      return;
    }
    if (verts[parent_id].child_count >= max_branches_per_node) {
      return;
    }

    i32 child_id = verts.size();
    Node child = {
      .pos = pos,
      .display_pos = verts[parent_id].display_pos + pos,
      .child_count = 0,
      .parent = parent_id,
      .size = 0.1f,
      .depth = verts[parent_id].depth + 1,
    };
    verts.push_back(child);
    edges.push_back({
      .start = parent_id,
      .end = child_id
    });

    verts[parent_id].children[verts[parent_id].child_count++] = child_id;
    verts[parent_id].size--;
  }

  void grow(FastNoiseLite &noise, float dt = 0.16) {

    i32 c = verts.size();
    // special case for the seed
    if (c == 1) {
      branch(0, vec3(0, 5.0f, 0));
      return;
    }

    f32 nutrients = 1000.0f;
    f32 nutrients_per_unit_area = 0.5f;
    f32 branching_size = 1.0f;
    f32 branching_length = 10.0f;
    // 1.0 is an even split
    f32 branching_factor = 1.0f;
    f32 branch_efficiency = 0.98f;

    u32 max_depth = 4;

    queue<i32> node_queue;
    node_queue.push(0);
    while(node_queue.size()) {
      i32 id = node_queue.front();
      node_queue.pop();

      if (nutrients < 0.0f) {
        break;
      }

      if (verts[id].depth >= max_depth) {
        break;
      }
      verts[id].size += 0.005;

      for (i32 i=0; i<verts[id].child_count; i++) {
        i32 child_id = verts[id].children[i];
        if (child_id < 0 || child_id >= verts.size()) {
          continue;
        }

        // compute the area
        f32 v = (f32(verts[child_id].size * 2) + f32(verts[id].size * 2)) / 2.0f;
        f32 l = length(verts[child_id].pos);
        f32 area = v * l;
        vec3 dir = normalize(verts[child_id].pos);

        if (!verts[child_id].child_count) {
          verts[child_id].pos += dir * dt;
          nutrients -= 1;
        } else {
          //verts[child_id].pos += dir * 0.0125f;
          nutrients -= area * nutrients_per_unit_area;
          nutrients *= branch_efficiency;
        }

        if (verts[child_id].child_count < max_branches_per_node - 1) {
          if (l > branching_length) {
            grow_tick+=0.001f;
            f32 random_angle = (noise.GetNoise(
              grow_tick * 100.0f,
              50.0f
            ) * 2.0f - 1.0f) * glm::pi<f32>() * 0.025f;

            // TODO: do this for more than a pair
            // TODO: utilize branching factor

            // vec2 r = glm::rotate(vec2(1.0, 0.0), random_angle);

            // vec3 adir = glm::rotate(dir,  random_angle, vec3(r.x, 0, 0)) + glm::rotate(dir,  random_angle, vec3(0, 0, r.y));
            // vec3 bdir = glm::rotate(dir, -random_angle, vec3(r.x, 0, 0) + glm::rotate(dir, -random_angle, vec3(0, 0, r.y)));
            vec3 adir = glm::sphericalRand(1.0);
            vec3 bdir = glm::sphericalRand(1.0);

            // TODO: randomize the lengths
            // if (true || next_random() < 0.05f) {
              branch(child_id, dir + adir * 0.25f);
            //}
            // if (true || next_random() < 0.05f) {
              branch(child_id, dir + bdir * 0.25f);
            // }
          }
        }

        if (verts[child_id].child_count) {
          node_queue.push(child_id);
        }
      }
    }
  }
};