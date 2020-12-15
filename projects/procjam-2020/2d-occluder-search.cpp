#include <rawkit/rawkit.h>
#include "glm/glm.hpp"
#include "glm/gtx/compatibility.hpp"
using namespace glm;

#define TAU 6.2831853

const uint32_t grid_diameter = 16;
#define occluder_list_len (2 * grid_diameter)
#define volume_len (grid_diameter * grid_diameter)
uint8_t volume[volume_len];
const vec2 grid_dims = {grid_diameter, grid_diameter};

void setup() {

  // fill the volume
  vec2 pos(0, 0);
  for (pos.x = 0.0f; pos.x<grid_dims.x; pos.x++) {
    for (pos.y = 0.0f; pos.y<grid_dims.y; pos.y++) {
      uint32_t loc = (
        static_cast<uint32_t>(pos.x) +
        static_cast<uint32_t>(pos.y * grid_dims.x)
      );

      float dist = glm::distance(pos + 0.5f, grid_dims * 0.5f);
      if (dist < (float)grid_diameter * 0.2f) {
        volume[loc] = 255;
      } else {
        volume[loc] = 0;
      }
    }
  }
}

void draw_camera(rawkit_vg_t *vg, vec2 eye, vec2 target) {
  // TODO: draw a camera that points at the target
  rawkit_vg_fill_color(vg, rawkit_vg_RGB(255, 255, 255));
  rawkit_vg_begin_path(vg);
  rawkit_vg_arc(vg, eye.x, eye.y, 0.5f, 0.0f, TAU, 1);
  rawkit_vg_fill(vg);
}

struct edge_lookup {
  vec2 start;
  vec2 end;
  vec2 normal;
};

uint8_t fire_ray(rawkit_vg_t *vg, vec2 eye, vec2 pos) {
  vec2 dir = glm::normalize(pos - eye);
  vec2 mapPos = vec2(floor(pos));
  vec2 deltaDist = abs(vec2(length(dir)) / dir);
  vec2 rayStep = sign(dir);

  vec2 sideDist = (sign(dir) * (mapPos - pos) + (sign(dir) * 0.5f) + 0.5f) * deltaDist;
  vec2 mask = glm::step(sideDist, vec2(sideDist.y, sideDist.x));

  float max_iterations = grid_diameter * 3.0;
  int hit_count = 0;
  for (int iterations = 0; iterations < max_iterations; iterations++) {

    if (all(greaterThanEqual(mapPos, vec2(0))) && all(lessThan(mapPos, grid_dims))) {
      uint32_t loc = (
        static_cast<uint32_t>(mapPos.x) +
        static_cast<uint32_t>(mapPos.y * grid_dims.x)
      );

      if (volume[loc]) {
        rawkit_vg_fill_color(vg, rawkit_vg_RGBA(64, 127, 64, 255));
        rawkit_vg_begin_path(vg);
          rawkit_vg_rect(vg, mapPos.x, mapPos.y, 0.75, 0.75);
          rawkit_vg_fill(vg);
        return volume[loc];
      } else {
        rawkit_vg_fill_color(vg, rawkit_vg_RGBA(255, 0, 0, 64));
        rawkit_vg_begin_path(vg);
          rawkit_vg_rect(vg, mapPos.x, mapPos.y, 0.75, 0.75);
          rawkit_vg_fill(vg);
      }
    }
    mask = glm::step(sideDist, vec2(sideDist.y, sideDist.x));
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }
  return 0;
}

void trace_edge(rawkit_vg_t *vg, vec2 eye, edge_lookup *edge) {
  vec2 start = glm::min(grid_dims - 1.0f, edge->start);
  vec2 end = glm::min(grid_dims - 1.0f, edge->end);
  vec2 diff = end - start;
  int comp = diff.x == 0.0f ? 1 : 0;
  float dir = diff.x == 0.0f ? sign(diff.y) : sign(diff.x);
  float step = glm::distance(start, end)/(float)grid_diameter;

  igText("start(%0.3f, %0.3f) -> end(%0.3f, %0.3f) step: %0.3f, dist: %f",
    start.x,
    start.y,
    end.x,
    end.y,
    step,
    glm::distance(start, end)
  );

  vec2 p = start;
  for (float i=0; i<(float)grid_diameter; i++) {
    p[comp] = start[comp] + dir * i;


    rawkit_vg_fill_color(vg, rawkit_vg_RGBA(255, 255, 255, 64));
    rawkit_vg_begin_path(vg);
      rawkit_vg_rect(vg, p.x, p.y, 0.75, 0.75);
      rawkit_vg_fill(vg);

    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(64, 64, 64));
    rawkit_vg_begin_path(vg);
      rawkit_vg_move_to(vg, eye.x, eye.y);
      rawkit_vg_line_to(vg, floor(p.x) + 0.5, floor(p.y) + 0.5);
      rawkit_vg_stroke(vg);


    // fire_ray(vg, eye, p);
    // fire_ray(vg, eye, p + vec2(1.0, 0.0));
    // fire_ray(vg, eye, p + vec2(1.0, 1.0));
    // fire_ray(vg, eye, p + vec2(0.0, 1.0));
    fire_ray(vg, eye, p + vec2(0.5));
  }

}

typedef struct state_t {
  float t;
} state_t;

void loop() {
  vec2 window_dims(
    rawkit_window_width(),
    rawkit_window_height()
  );
  rawkit_vg_t *vg = rawkit_default_vg();
  rawkit_vg_translate(vg, window_dims.x/2.0f, window_dims.y/2.0f);
  rawkit_vg_scale(vg, 16.0f, -16.0f);
  rawkit_vg_translate(vg, -grid_dims.x/2.0f, -grid_dims.y/2.0f);

  rawkit_vg_stroke_width(vg, 1.0f/16.0f);

  float now = rawkit_now();
  state_t *state = rawkit_hot_state("state_t", state_t);


  {
    float dmin = 0.0f;
    float dmax = 10.0f;
    igSliderScalar("##camera-time", ImGuiDataType_Float, &state->t,  &dmin, &dmax, "time %f", 1.0f);
  }

  now = state->t;
  vec2 eye = vec2(
    sin(now) * 16.0f,
    cos(now) * 16.0f
  ) + grid_dims * 0.5f;

  vec2 target = grid_dims * 0.5f;
  draw_camera(vg, eye, target);

  uvec2 occluder_list[occluder_list_len] = {};

  // render the grid
  {
    vec2 pos(0, 0);
    for (pos.x = 0.0f; pos.x<grid_dims.x; pos.x++) {
      for (pos.y = 0.0f; pos.y<grid_dims.y; pos.y++) {
        rawkit_vg_begin_path(vg);
        rawkit_vg_rect(vg, pos.x, pos.y, 0.75f, 0.75f);

        uint32_t loc = (
          static_cast<uint32_t>(pos.x) +
          static_cast<uint32_t>(pos.y * grid_dims.x)
        );
        if (volume[loc]) {
          rawkit_vg_stroke_color(vg, rawkit_vg_RGB(64, 127, 64));
        } else {
          rawkit_vg_stroke_color(vg, rawkit_vg_RGB(127, 127, 127));
        }
        rawkit_vg_stroke(vg);
      }
    }
  }

  // render the edges to be searched
  {

    edge_lookup edges[4] = {
      {
        .start = vec2(0.0f, 0.0f),
        .end = vec2(grid_dims.x, 0.0f),
        .normal = vec2(0.0f, -1.0f),
      },
      {
        .start = vec2(grid_dims.x, 0.0f),
        .end = vec2(grid_dims.x, grid_dims.y),
        .normal = vec2(1.0f, 0.0f),
      },
      {
        .start = vec2(grid_dims.x, grid_dims.y),
        .end = vec2(0.0f, grid_dims.y),
        .normal = vec2(0.0f, 1.0f),
      },
      {
        .start = vec2(0.0f, grid_dims.y),
        .end = vec2(0.0f, 0.0f),
        .normal = vec2(-1.0f, 0.0f),
      },
    };

    // TODO: if inside the grid, the behavior changes
    // TODO: there is a faster way to compute which edges should be sampled..
    vec2 dir = normalize(eye - target);

    for (int i=0; i<4; i++) {
      edge_lookup *edge = &edges[i];
      float r = glm::dot(dir, edge->normal);

      if (r >= 0.5f) {
        rawkit_vg_stroke_color(vg, rawkit_vg_RGBf(r, 0, r));
        rawkit_vg_begin_path(vg);
          rawkit_vg_move_to(vg, edge->start.x + edge->normal.x, edge->start.y + edge->normal.y);
          rawkit_vg_line_to(vg, edge->end.x + edge->normal.x, edge->end.y + edge->normal.y);
          rawkit_vg_stroke(vg);

        trace_edge(vg, eye, edge);
      }
    }



  }
}