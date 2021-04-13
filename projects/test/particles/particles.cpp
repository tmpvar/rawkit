#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
using namespace glm;

#include "context-2d.h"
#include "shared.h"

#define MAX_PARTICLES_PER_CELL 8
#define MAX_PARTICLES 1000000
#define MAX_NEIGHBORS 9 * MAX_PARTICLES_PER_CELL
static u32 tmp_neighbors[MAX_NEIGHBORS];

#define QUAD_INDICES_COUNT 6
#define QUAD_VERTICES_COUNT 4
#define INDICES_SIZE QUAD_INDICES_COUNT * MAX_PARTICLES * sizeof(u32)


struct State {
  Scene scene;

  vec2 *particles;
  u8 *cell_idx;
  u32 *cells;
  rawkit_gpu_buffer_t *index_buffer;
  rawkit_gpu_ssbo_t *particle_ssbo;
  double last_time;
};

void setup(){
  auto state = rawkit_hot_state("state", State);
  state->last_time = rawkit_now();
  state->scene.dims = vec2(
    rawkit_window_width(),
    rawkit_window_height()
  );

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

  for (u32 i=0; i<10; i++) {
    sb_push(state->particles, vec2(
      rawkit_randf(),
      rawkit_randf()
    ) * state->scene.dims);
  }
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

  float radius = RADIUS;//state->scene.radius;
  float diameter = radius * 2.0f;

  if (igIsMouseDown(ImGuiMouseButton_Left)) {
    ImVec2 mouse;
    igGetMousePos(&mouse);
    sb_push(state->particles, vec2(mouse.x,state->scene.dims.y - mouse.y));
  }

  u32 particle_count = sb_count(state->particles);

  // gravity
  {
    for (u32 i=0; i<particle_count; i++) {
      vec2 &p = state->particles[i];
      p.y -= dt * 9.8f;
    }
  }

  // boundaries
  {
    for (u32 i=0; i<particle_count; i++) {
      state->particles[i] = clamp(
        state->particles[i],
        vec2(radius),
        state->scene.dims - radius
      );
    }
  }

  // particle vs particle
  {
    // resize + reset world grid
    uvec2 cells(glm::ceil(state->scene.dims / 100.0f));

    u32 width = cells.x * MAX_PARTICLES_PER_CELL;
    u32 count = cells.x * cells.y;
    state->cell_idx = (u8 *)realloc(state->cell_idx, count);
    memset(state->cell_idx, 0, count);
    state->cells = (u32 *)realloc(state->cells, count * sizeof(u32) * MAX_PARTICLES_PER_CELL);

    // dump the particles into a grid
    {
      for (u32 i=0; i<particle_count; i++) {
        vec2 &p = state->particles[i];
        for (u8 attempts = 0; attempts<1; attempts++) {
          u32 idx = (u32)floor(p.x) + (u32)floor(p.y) * cells.x;
          u32 offset = state->cell_idx[idx];
          if (offset >= MAX_PARTICLES_PER_CELL) {
            // p.y -= radius * 2.0;
            continue;
          }

          u32 cell_idx = offset + (u32)floor(p.x) * MAX_PARTICLES_PER_CELL + (u32)floor(p.y) * cells.x * MAX_PARTICLES_PER_CELL;
          if (cell_idx > count * MAX_PARTICLES_PER_CELL) {
            continue;
          }

          state->cells[cell_idx] = i;
          state->cell_idx[idx]++;
          break;
        }
      }
    }

    // cell based checks
    if (0) {
      memset(tmp_neighbors, 0, sizeof(tmp_neighbors));


      for (u32 x=1; x<cells.x-1; x+=2) {
        for (u32 y=1; y<cells.y-1; y+=2) {
          u32 dst_idx = 0;
          for (u32 ny=y-1; ny<=y+1; ny++) {
            for (u32 nx=x-1; nx<=x+1; nx++) {
              u32 cell_particle_count = state->cell_idx[nx + ny * cells.x];
              u32 src_idx = nx * MAX_PARTICLES_PER_CELL + ny * width;
              memcpy(&tmp_neighbors[dst_idx], &state->cells[src_idx], sizeof(u32) * cell_particle_count);
              dst_idx += cell_particle_count;
            }
          }

          for (u32 i=0; i<dst_idx; i++) {
            for (u32 j=0; j<dst_idx; j++) {
              if (i == j) {
                continue;
              }

              const vec2 &a = state->particles[i];
              const vec2 &b = state->particles[j];

              vec2 diff = a - b;
              if (diff.x == 0.0f) {
                diff.x = radius;
              }

              if (diff.y == 0.0f) {
                diff.y = radius;
              }
              float d = length(diff) - diameter;
              if (d >= 0.0) {
                continue;
              }

igText("diff(%f, %f) d(%f)", diff.x, diff.y, d);
              state->particles[i] += diff * 2.0f;// * 0.5f;
              state->particles[j] -= diff * 2.0f;// * 0.5f;

              state->particles[i] = clamp(
                state->particles[i],
                vec2(radius),
                state->scene.dims - radius
              );

              state->particles[j] = clamp(
                state->particles[j],
                vec2(radius),
                state->scene.dims - radius
              );
            }
          }
        }
      }
    }


    // brute force
    {
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

            if (d > 0.0f) {
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

    // u32 l = sb_count(state->particles);
    // for (u32 i=0; i<particle_count; i++) {
    //   const vec2 &p = state->particles[i];
    //   ctx.beginPath();
    //     ctx.arc(p, radius);
    //     ctx.fill();
    // }
  }

}