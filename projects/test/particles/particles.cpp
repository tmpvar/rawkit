#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <glm/glm.hpp>
using namespace glm;

#include <morton/morton.h>
using namespace libmorton;

#include <roaring/roaring.h>

#include "shared.h"
#include "prof.h"

#define MAX_PARTICLES_PER_CELL 26
#define MAX_PARTICLES 1000000
#define MAX_NEIGHBORS 9 * MAX_PARTICLES_PER_CELL
static u32 tmp_neighbors[MAX_NEIGHBORS];

#define QUAD_INDICES_COUNT 6
#define QUAD_VERTICES_COUNT 4
#define INDICES_SIZE QUAD_INDICES_COUNT * MAX_PARTICLES * sizeof(u32)

struct Cell {
  u32 size = 0;
  u32 data[MAX_PARTICLES_PER_CELL];
  void reset() {
    this->size = 0;
  }

  bool push(u32 particle_idx) {
    if (this->size >= MAX_PARTICLES_PER_CELL) {
      return false;
    }

    this->data[this->size++] = particle_idx;
    return true;
  }
};

#define SCREEN_TO_GRID 1.0f / (f32)(DIAMETER)

struct Grid {
  uvec2 dims;
  u32 cell_count;
  Cell *cells = nullptr;
  roaring_bitmap_t *active_cells;

  Grid() {
    this->active_cells = roaring_bitmap_create();
  }

  void tick(const vec2 &screen_dims) {
    this->dims = uvec2( screen_dims * SCREEN_TO_GRID);
    u32 next_cell_count = dims.x * dims.y;

    if (this->cell_count != next_cell_count) {
      this->cell_count = next_cell_count;
      this->cells = (Cell *)realloc(this->cells, next_cell_count * sizeof(Cell));

    }

    for (u32 i=0; i<this->cell_count; i++) {
      this->cells[i].reset();
    }

    roaring_bitmap_clear(this->active_cells);

    igText("cells: %u, active_cells: %u",
      this->cell_count,
      roaring_bitmap_get_cardinality(this->active_cells)
    );

  }

  uvec2 grid_pos(const vec2 &pos) {
    return uvec2(
      (pos.x * SCREEN_TO_GRID),
      (pos.y * SCREEN_TO_GRID)
    );
  }

  u32 grid_idx(const uvec2 &pos) {
    uint_fast32_t x = static_cast<uint_fast32_t>(pos.x);
    uint_fast32_t y = static_cast<uint_fast32_t>(pos.y);
    return morton2D_32_encode(x, y);
  }

  void add(const vec2 &fpos, u32 particle_idx) {
    uvec2 pos = this->grid_pos(fpos);
    add(pos, particle_idx);
  }

  void add(const uvec2 &pos, u32 particle_idx) {
    uint_fast32_t cell_idx = this->grid_idx(pos);
    this->add(cell_idx, particle_idx);
  }

  void add(u32 cell_idx, u32 particle_idx) {
    if (cell_idx >= cell_count) {
      return;
    }

    Cell *c = this->cell(cell_idx);
    if (!c) {
      return;
    }

    if (c->push(particle_idx)) {
      roaring_bitmap_add(this->active_cells, cell_idx);
    }
  }

  Cell *cell(u32 idx) {
    if (idx >= this->cell_count) {
      return nullptr;
    }
    return &this->cells[idx];
  }
};

struct Constraint {
  u32 a;
  u32 b;
  vec2 normal;

  Constraint(u32 a, u32 b, const vec2 &normal)
  : a(a), b(b), normal(normal)
  {
    // igText("constraint(%u, %u, normal(%f, %f))", a, b, normal.x, normal.y);
  }
};

struct State {
  Scene scene;

  vec2 *particles;
  rawkit_gpu_buffer_t *index_buffer;
  rawkit_gpu_ssbo_t *particle_ssbo;
  double last_time;

  Grid *grid;

  Constraint *constraints;

  // Deprecated
  u32 cell_idx_size;
  u32 cells_size;
  u8 *cell_idx;
  u32 *cells;

};

void setup(){
  auto state = rawkit_hot_state("state", State);
  state->last_time = rawkit_now();
  state->scene.dims = vec2(
    rawkit_window_width(),
    rawkit_window_height()
  );

  if (!state->grid) {
    state->grid = new Grid();
  }

  if (!state->index_buffer) {
    u32 quad_indices[6] = {
      0, 2, 1, 2, 3, 1,
    };

    state->index_buffer = rawkit_gpu_buffer_create(
      "index_buffer",
      rawkit_default_gpu(),
      INDICES_SIZE,
      (
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT
      ),
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    u32 *mem = (u32 *)malloc(INDICES_SIZE);
    u32 len = QUAD_INDICES_COUNT * MAX_PARTICLES;
    for (u32 i=0; i<len; i++) {
      u32 quad = i / QUAD_INDICES_COUNT;
      u32 index = i % QUAD_INDICES_COUNT;
      mem[i] = quad_indices[index] + quad * QUAD_VERTICES_COUNT;
    }

    rawkit_gpu_buffer_update(state->index_buffer, mem, INDICES_SIZE);

    free(mem);
  }

  state->particle_ssbo = rawkit_gpu_ssbo("particles", sizeof(vec2) * MAX_PARTICLES);
  if (sb_count(state->particles) == 0) {
    for (u32 i=0; i<2; i++) {
      sb_push(state->particles, vec2(
        rawkit_randf(),
        rawkit_randf()
      ) * state->scene.dims);
    }
  }
}

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

void loop() {
  auto state = rawkit_hot_state("state", State);

  double now = rawkit_now();
  float dt = (now - state->last_time) * 10.0f;
  state->last_time = now;
  state->scene.dims = vec2(
    rawkit_window_width(),
    rawkit_window_height()
  );

  state->grid->tick(state->scene.dims);

  float radius = RADIUS;
  float inv_radius = 1.0 / RADIUS;
  float diameter = radius * 2.0f;
  float inv_cell_size = 1.0 / diameter;
  ImVec2 mouse;
  igGetMousePos(&mouse);

  if (igIsMouseDown(ImGuiMouseButton_Left)) {
    for (u32 i=0; i<10; i++) {

      sb_push(state->particles, vec2(mouse.x,state->scene.dims.y - mouse.y) + vec2(
        rawkit_randf() * 100.0f - 50.0f,
        rawkit_randf() * 100.0f - 50.0f
      ));
    }


  }
  state->particles[0] = vec2(mouse.x,state->scene.dims.y - mouse.y);


  u32 particle_count = sb_count(state->particles);
  igText("particles: %u", particle_count);
  igText("dt: %f", dt);

  // gravity
  if (0) {
    Prof p("gravity");
    for (u32 i=0; i<particle_count; i++) {
      vec2 &p = state->particles[i];
      p.y -= dt * 9.8f;
    }
  }

  // boundaries
  if (1) {
    for (u32 i=0; i<particle_count; i++) {
      state->particles[i] = clamp(
        state->particles[i],
        vec2(radius * 4.0f),
        state->scene.dims - radius * 4.0f
      );
    }
  }

  // fill grid
  {
    Prof p("fill grid");
    for (u32 i=0; i<particle_count; i++) {
      vec2 start = state->particles[i];
      uvec2 center_pos = state->grid->grid_pos(start);
      u32 center_idx = state->grid->grid_idx(center_pos);
      state->grid->add(center_idx, i);
      for (float x=-1.0f; x<=1.0f; x+=2.0f) {
        for (float y=-1.0f; y<=1.0f; y+=2.0f) {

          uvec2 pos = state->grid->grid_pos(
            start + vec2(x * RADIUS, y * RADIUS)
          );
          u32 idx = state->grid->grid_idx(pos);
          if (idx == center_idx) {
            continue;
          }

          state->grid->add(idx, i);
        }
      }
    }
  }

  igText("active_cells: %u",
    roaring_bitmap_get_cardinality(state->grid->active_cells)
  );

  // add constraints (Grid)
  if (0) {
    u32 l = state->grid->cell_count;
    igText("cells(%u)", l);
    Prof p("add constraints (Grid)");
    sb_reset(state->constraints);
    u32 active = 0;

    u32 cell_idx=0;
    for (u32 cell_idx=0; cell_idx < l; cell_idx++) {
      if (!roaring_bitmap_contains(state->grid->active_cells, cell_idx)) {
        continue;
      }

      Cell *cell = state->grid->cell(cell_idx);

      if (!cell || !cell->size) {
        continue;
      }

      active++;
      u32 cell_particle_count = cell->size;

      for (u32 i=0; i<cell_particle_count; i++) {
        u32 a_idx = cell->data[i];
        vec2 &a = state->particles[a_idx];
        for (u32 j=i+1; j<cell_particle_count; j++) {
          u32 b_idx = cell->data[j];

          if (a_idx == b_idx) {
            continue;
          }

          vec2 &b = state->particles[b_idx];
          vec2 diff = a - b;
          if (diff.x == 0.0f) {
            diff.x = 0.000001f;
          }

          if (diff.y == 0.0f) {
            diff.y = 0.000001f;
          }

          float d = length(diff);
          if (d <= DIAMETER) {

            sb_push(
              state->constraints,
              Constraint(a_idx, b_idx, normalize(diff))
            );
          }
        }
      }
    }
  }

  else

  {
    u32 l = state->grid->cell_count;
    igText("cells(%u)", l);
    Prof p("add constraints (Brute Force)");
    sb_reset(state->constraints);

    for (u32 i=0; i<particle_count; i++) {
      vec2 &a = state->particles[i];
      for (u32 j=i+1; j<particle_count; j++) {
        vec2 &b = state->particles[j];
        vec2 diff = a - b;
        if (diff.x == 0.0f) {
          diff.x = 0.000001f;
        }

        if (diff.y == 0.0f) {
          diff.y = 0.000001f;
        }

        float d = length(diff);
        if (d <= DIAMETER) {

          sb_push(
            state->constraints,
            Constraint(i, j, normalize(diff))
          );
        }
      }

    }
  }

  // solve constraints
  {
    Prof p("solve");
    u32 constraint_count = sb_count(state->constraints);
    igText("constraints: %u", constraint_count);
    for (u32 steps=0; steps<10; steps++) {
      for (u32 i=0; i<constraint_count; i++) {
        const Constraint &constraint = state->constraints[i];
        vec2 &a = state->particles[constraint.a];
        vec2 &b = state->particles[constraint.b];

        float d = (distance(a, b) - DIAMETER) * 0.5f;
        a -= constraint.normal * d;
        b += constraint.normal * d;

        // b = clamp(
        //   b,
        //   vec2(radius * 4.0f),
        //   state->scene.dims - radius * 4.0f
        // );
        // a = clamp(
        //   a,
        //   vec2(radius * 4.0f),
        //   state->scene.dims - radius * 4.0f
        // );
      }
    }
  }

  // particle vs particle
  if (0) {
    // resize + reset world grid
    uvec2 cells(glm::ceil(state->scene.dims * inv_cell_size));

    u32 count = cells.x * cells.y;

    u32 cell_idx_size = next_power_of_two(cells.x * cells.y);
    u32 cells_size = next_power_of_two(count * MAX_PARTICLES_PER_CELL * sizeof(u32));
    igText("cells(%u) dims(%u, %u)", cells.x * cells.y, cells.x, cells.y);

    if (state->cell_idx_size < cell_idx_size) {
      printf("realloc cell_idx(%u)\n", cell_idx_size);
      state->cell_idx_size = cell_idx_size;
      state->cell_idx = (u8 *)realloc(state->cell_idx, cell_idx_size);
    }
    memset(state->cell_idx, 0, state->cell_idx_size);

    if (state->cells_size < cells_size) {
      printf("realloc cells(%u)\n", cells_size);
      state->cells = (u32 *)realloc(state->cells, cells_size);
      state->cells_size = cells_size;
    }

    // dump the particles into a grid
    {
      Prof prof("particle->grid");
      u32 skipped = 0;
      for (u32 i=0; i<particle_count; i++) {
        vec2 &p = state->particles[i];
        for (u8 attempts = 0; attempts<10; attempts++) {

          u32 x = (u32)(p.x * inv_cell_size);
          u32 y = (u32)(p.y * inv_cell_size);
          if (x >= cells.x || y >= cells.y) {
            skipped++;
            break;
          }

          auto idx = morton2D_32_encode(x, y);
          u32 offset = state->cell_idx[idx];

          if (offset >= MAX_PARTICLES_PER_CELL) {
            // p -= (vec2(rawkit_randf(), rawkit_randf()) - 0.5f) * radius;
            p.y += diameter;
            continue;
          }

          u32 cell_idx = offset + idx * MAX_PARTICLES_PER_CELL;
          state->cells[cell_idx] = i;
          state->cell_idx[idx]++;
          break;
        }
      }
      igText("skipped(%u)", skipped);
    }

    // cell based checks
    if (1) {
      Prof prof("cells");
      for (u32 steps=0; steps<20; steps++) {
        for (u32 y=1; y<=cells.y-1; y+=2) {
          for (u32 x=1; x<=cells.x-1; x+=2) {
            u32 dst_idx = 0;
            for (uint_fast32_t ny=y-1; ny<=y+1; ny++) {
              for (uint_fast32_t nx=x-1; nx<=x+1; nx++) {
                u32 idx = morton2D_32_encode(nx, ny);
                u32 cell_particle_count = state->cell_idx[idx];
                u32 src_idx = idx * MAX_PARTICLES_PER_CELL;
                if (dst_idx >= cell_idx_size) {
                  break;
                }
                memcpy(&tmp_neighbors[dst_idx], &state->cells[src_idx], sizeof(u32) * cell_particle_count);
                dst_idx += cell_particle_count;
              }
            }


            for (u32 i=0; i<dst_idx; i++) {
              u32 a_idx = tmp_neighbors[i];
              if (a_idx > particle_count) {
                continue;
              }
              vec2 &a = state->particles[a_idx];
              a = clamp(
                a,
                vec2(radius * 4.0f),
                state->scene.dims - radius * 4.0f
              );
              for (u32 j=0; j<dst_idx; j++) {
                if (j == i) {
                  continue;
                }

                u32 b_idx = tmp_neighbors[j];
                if (b_idx > particle_count) {
                  continue;
                }

                vec2 &b = state->particles[tmp_neighbors[j]];
                b = clamp(
                  b,
                  vec2(radius * 4.0f),
                  state->scene.dims - radius * 4.0f
                );
                float d = diameter - distance(a, b);

                if (d <= 0.0f || isnan(d)) {
                  continue;
                }

                vec2 diff = b - a;
                if (diff.x == 0.0f) {
                  diff.x = 0.00001f;
                }
                if (diff.y == 0.0f) {
                  diff.y = 0.00001f;
                }

                diff = normalize(diff);
                d = d * 0.5f;

                b += diff * d * 0.5f;
                a += diff * d * -0.5f;

                b = clamp(
                  b,
                  vec2(radius * 4.0f),
                  state->scene.dims - radius * 4.0f
                );
                a = clamp(
                  a,
                  vec2(radius * 4.0f),
                  state->scene.dims - radius * 4.0f
                );

              }
            }
          }
        }
      }
    }

    // brute force
    if (0) {
      Prof prof("brute force");
      for (u32 step=0; step<20; step++) {
        for (u32 i=0; i<particle_count; i++) {
          state->particles[i] = clamp(
            state->particles[i],
            vec2(radius),
            state->scene.dims - radius
          );
        }

        for (u32 i=0; i<=particle_count; i++) {
          vec2 &a = state->particles[i];
          for (u32 j=i+1; j<particle_count; j++) {
            vec2 &b = state->particles[j];

            if (a.x == b.x) {
              b.x -= 0.0000001f;
            }

            if (a.y == b.y) {
              b.y += 0.0000001;
            }

            float d = distance(a, b) - diameter;

            if (d >= 0.0f) {
              continue;
            }
            vec2 diff = a - b;
            diff = normalize(diff);

            a -= diff * d;// * 0.5f;// * d * 0.5f;
            b += diff * d;// * 0.5f;// * d * 0.5f;
            continue;
            // float d = length(d) - diameter;
            // state->particles[i] -= diff * d * 0.5f * 0.1F;// * d * 0.5f;
            // state->particles[j] += diff * d * 0.5f * 0.1F;// * d * 0.5f;
          }
        }
      }
    }
  }

  // render the particles
  {
    rawkit_gpu_ssbo_update(
      state->particle_ssbo,
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      state->particles,
      sizeof(vec2) * particle_count
    );

    rawkit_shader_options_t options = rawkit_shader_default_options();
    options.alphaBlend = true;
    options.depthTest = false;

    auto shader = rawkit_shader_opts(
      options,
      rawkit_file("particles.vert"),
      rawkit_file("particles.frag")
    );

    auto inst = rawkit_shader_instance_begin(shader);
    if (inst) {

      rawkit_shader_instance_param_ssbo(inst, "Particles", state->particle_ssbo);
      rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);

      VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = state->scene.dims.x,
        .height = state->scene.dims.y,
        .minDepth = 0.0,
        .maxDepth = 1.0
      };

      vkCmdSetViewport(
        inst->command_buffer,
        0,
        1,
        &viewport
      );

      VkRect2D scissor = {};
      scissor.extent.width = viewport.width;
      scissor.extent.height = viewport.height;
      vkCmdSetScissor(
        inst->command_buffer,
        0,
        1,
        &scissor
      );

      vkCmdBindIndexBuffer(
        inst->command_buffer,
        state->index_buffer->handle,
        0,
        VK_INDEX_TYPE_UINT32
      );

      vkCmdDrawIndexed(
        inst->command_buffer,
        particle_count * QUAD_INDICES_COUNT,
        1,
        0,
        0,
        1
      );

      rawkit_shader_instance_end(inst);
    }
  }
}