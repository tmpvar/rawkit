#pragma once

#include <rawkit/rawkit.h>
#include "aabb.h"
#include "sdf.h"
#include "polygon.h"
#include "segseg.h"

#include <glm/glm.hpp>
using namespace glm;

#include <queue>
using namespace std;

static char blob_tmp_str[4096] = "";

struct PackedCircle {
  vec2 pos;
  float radius;
  float signed_distance;
  bool skip;
};

struct Edge {
  uint32_t a;
  uint32_t b;
};

struct Node {
  uint32_t start;
  uint32_t len;
};

struct NodeQueue {
  uint32_t *array = nullptr;
  uint32_t loc = 0;

  ~NodeQueue() {
    sb_free(this->array);
  }

  void reset() {
    this->loc = 0;
    sb_reset(this->array);
  }

  void push(uint32_t v) {
    sb_push(this->array, v);
  }

  uint32_t pop() {
    return this->array[this->loc++];
  }

  int pending() {
    return sb_count(this->array) - this->loc;
  }

  bool done() {
    return this->loc >= sb_count(this->array);
  }
};

struct Blob {
  char *name = nullptr;
  vec2 dims;
  // TODO: add translation + rotation
  SDF *sdf = nullptr;
  PackedCircle *circles = nullptr;
  Edge *circle_edges = nullptr;
  Node *circle_nodes = nullptr;

  Blob(const char *name, vec2 dims) : dims(dims) {
    this->name = (char *)calloc(strlen(name)+1, 1);
    strcpy(this->name, name);

    this->sdf = new SDF(this->name, vec3(dims, 0.0));
  }

  Blob(const char *name) {
    this->name = (char *)calloc(strlen(name)+1, 1);
    strcpy(this->name, name);
  }

  ~Blob() {
    delete this->sdf;
  }


  Blob **slice_with_line(vec2 a, vec2 b) {

    // create two blobs for the left and right of the line
    sprintf(blob_tmp_str, "%s-slice-left", this->name);
    Blob *left = new Blob(blob_tmp_str, this->dims);

    sprintf(blob_tmp_str, "%s-slice-right", this->name);
    Blob *right = new Blob(blob_tmp_str, this->dims);


    // TODO: transform into local space once we start rotating + translating
    vec2 p;
    for (p.x=0.0f; p.x<this->dims.x; p.x++) {
      for (p.y=0.0f; p.y<this->dims.y; p.y++) {
        float o = orientation(a, b, p + 0.5f);
        vec2 closest = line_closest_point(a, b, p + 0.5f);
        float ldist = distance(closest, p + 0.5f);
        uvec2 grid_pos(p);
        right->sdf->write(grid_pos, glm::max(ldist * glm::sign(o), this->sdf->sample(p)));
        left->sdf->write(grid_pos, glm::max(ldist * -glm::sign(o), this->sdf->sample(p)));
      }
    }

    Blob **blobs = nullptr;
    sb_push(blobs, left);
    sb_push(blobs, right);
    return blobs;
  }

  static int sort_circles_by_radius(const void *ap, const void *bp) {
    const PackedCircle *a = (PackedCircle *)ap;
    const PackedCircle *b = (PackedCircle *)bp;

    // sort by validity first
    if (a->skip != b->skip) {
      return a->skip ? 1 : -1;
    }

    // order by radius desc (effectively by distance asc)
    return a->radius > b->radius ? -1 : 1;
  }

  void circle_graph() {
    sb_reset(this->circle_edges);
    sb_reset(this->circle_nodes);

    float eps = 1.25f;
    // TODO: use a heirachy of grids or hash
    uint32_t circle_count = sb_count(this->circles);
    for (uint32_t i=0; i<circle_count; i++) {
      PackedCircle a = this->circles[i];
      Node node = {
        .start = (uint32_t)sb_count(circle_edges),
        .len = 0
      };

      for (uint32_t j=0; j<circle_count; j++) {
        if (i == j) {
          continue;
        }

        PackedCircle b = this->circles[j];

        float d = distance(a.pos, b.pos);
        float r = a.radius + b.radius;

        if (d - r < eps) {
          node.len++;

          Edge edge = {
            .a = i,
            .b = j
          };
          sb_push(this->circle_edges, edge);
        }
      }

      sb_push(this->circle_nodes, node);
    }

    printf("circle_graph: %u edges\n", sb_count(this->circle_edges));
  }

  Blob **extract_islands() {
    Blob **islands = nullptr;

    // build the islands list
    {
      uint32_t circle_count = sb_count(this->circles);
      uint32_t edge_count = sb_count(this->circle_edges);
      // TODO: a bitset would work much better here.
      uint8_t *visited = (uint8_t *)calloc(circle_count, sizeof(uint8_t));
      NodeQueue queue = {};

      for (uint32_t visited_idx=0; visited_idx<circle_count; visited_idx++) {
        if (visited[visited_idx]) {
          continue;
        }

        sprintf(blob_tmp_str, "%s-island#%u", this->name, sb_count(islands));
        sb_push(islands, new Blob(blob_tmp_str));
        queue.push(visited_idx);

        while(!queue.done()) {
          uint32_t node_idx = queue.pop();
          if (visited[node_idx]) {
            continue;
          }
          visited[node_idx] = 1;

          sb_push(sb_last(islands)->circles, this->circles[node_idx]);

          Node node = this->circle_nodes[node_idx];

          for (uint32_t i=0; i<node.len; i++) {
            uint32_t next_edge_idx = node.start + i;
            Edge edge = this->circle_edges[next_edge_idx];
            uint32_t next = edge.b;
            queue.push(next);
          }
        }
      }
      free(visited);
    }

    // copy the associated sections of the SDF to each island
    {
      uint32_t island_count = sb_count(islands);
      for (uint32_t i=0; i<island_count; i++) {
        Blob *island = islands[i];
        AABB island_aabb;
        uint32_t circle_count = sb_count(island->circles);
        for (uint32_t c = 0; c<circle_count; c++) {
          PackedCircle circle = island->circles[c];
          island_aabb.grow(circle.pos + vec2(circle.radius));
          island_aabb.grow(circle.pos - vec2(circle.radius));
        }

        island_aabb.lb = floor(island_aabb.lb);
        island_aabb.ub = ceil(island_aabb.ub);

        island->dims = vec2(
          island_aabb.width(),
          island_aabb.height()
        );

        island->sdf = new SDF(island->name, vec3(island->dims, 0.0));
        vec2 p;
        for (p.x = 0.0; p.x<island->dims.x; p.x++) {
          for (p.y = 0.0; p.y<island->dims.y; p.y++) {

            float d = this->sdf->sample(p + island_aabb.lb);
            island->sdf->write(
              uvec2(p),
              this->sdf->sample(p + island_aabb.lb)
            );
          }
        }

        sb_reset(island->circles);
        island->circle_pack();

      }
    }

    return islands;
  }



  void circle_pack() {
    // compute a list of pixels that are inside the shape
    PackedCircle *inside = nullptr;
    printf("circle_pack: build inside list\n");
    {
      PackedCircle c = {};
      for (c.pos.x = 0.0f; c.pos.x < this->dims.x; c.pos.x++) {
        for (c.pos.y = 0.0f; c.pos.y < this->dims.y; c.pos.y++) {
          c.signed_distance = this->sdf->sample(c.pos);
          c.radius = abs(c.signed_distance);
          c.skip = false;
          if (c.signed_distance <= 0.0f) {
            sb_push(inside, c);
          }
        }
      }
    }

    printf("circle_pack: sort inside list\n");
    // sort the list by distance ascending
    uint32_t inside_count = sb_count(inside);
    {
      qsort(inside, inside_count, sizeof(PackedCircle), Blob::sort_circles_by_radius);
    }

    printf("circle_pack: add first circle\n");
    // place the first circle
    inside[0].skip = true;
    sb_push(this->circles, inside[0]);

    // loop through the rest of the circles and add them to the list if
    // the do not intersect any previously placed circle
    int sentinel = 100;
    while (inside_count && sentinel--) {
      for (uint32_t i=1; i<inside_count; i++) {
        PackedCircle circle = inside[i];
        uint32_t circle_count = sb_count(this->circles);
        bool skip = false;
        for (uint32_t cidx=0; cidx<circle_count; cidx++) {
          PackedCircle packed_circle = this->circles[cidx];
          float d = distance(circle.pos, packed_circle.pos) - packed_circle.radius;

          if (d < 0.0f) {
            inside[i].skip = true;
            skip = true;
            break;
          }

          if (d < circle.radius || skip) {
            inside[i].radius = glm::min(inside[i].radius, d);
            if (inside[i].radius < 0.5f) {
               inside[i].skip = true;
            }
            skip = true;
          }
        }

        if (skip || inside[i].radius < 1.0f) {
          continue;
        }

        inside[i].skip = true;
        sb_push(this->circles, inside[i]);
      }

      // sort the list to bring pending entries back to the top and then
      // resize the array
      {
        qsort(inside, inside_count, sizeof(PackedCircle), Blob::sort_circles_by_radius);
        for (uint32_t i=0; i<inside_count; i++) {
          if (inside[i].skip) {
            stb__sbn(inside) = i;
            inside_count = i;
            break;
          }
        }
      }
    }
  }

};