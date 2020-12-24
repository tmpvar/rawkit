#define CPU_HOST

#include <rawkit/rawkit.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/compatibility.hpp"
#include "glm/glm.hpp"
using namespace glm;

#include "bricks/shared.h"

#include "perlin.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
using namespace std;

// struct Brick {
//   // vec3 pos;
//   float x;
//   float y;
//   float z;
//   float w;
//   // uint64_t occlusion[64];
// };


enum class RebuildState {
  NONE,
  ACTIVE,
  COMPLETE,
};

#define RebuildStateAtomic atomic<RebuildState>::value_type

struct Cascade {
  uint32_t level;
  uint32_t count;
  Brick *cpu_bricks;
  rawkit_gpu_ssbo_t *gpu_bricks;

  BrickColor *cpu_colors = NULL;
  rawkit_gpu_ssbo_t *gpu_colors;

  vec3 center;

  // Rebuild tools
  mutex rebuild_mutex;
  atomic<int> rebuild_complete;
  Camera camera;
  thread pending;
};

#define CASCADE_COUNT 1
#define CASCADE_DIAMETER 96
#define INV_CASCADE_DIAMETER (1.0f/(float)CASCADE_DIAMETER * 0.1f)
struct state_t {
  Cascade cascades[1];
  Camera camera;
  Scene scene;
};

struct Prof {
  const char *name;
  double start = 0.0;
  Prof(const char *name)
    : name(name)
  {
    this->start = rawkit_now();
  }

  double diff_ms() {
    return (rawkit_now() - this->start) * 1000.0;
  }

  ~Prof() {
    printf("%s took %f ms", this->name, diff_ms());
  }
};

char tmp_str[512] = "\0";

void cascade_init(Cascade *cascade, uint32_t level) {
  uint64_t brick_count = pow(CASCADE_DIAMETER, 3);
  cascade->level = level;
  uint64_t total_bytes = 0;
  if (!cascade->cpu_bricks) {
    uint64_t byte_count = brick_count * sizeof(Brick);
    total_bytes += byte_count;
    cascade->cpu_bricks = (Brick *)calloc(byte_count, 1);
    sprintf(tmp_str, "cascade-level-%u-positions", level);
    cascade->gpu_bricks = rawkit_gpu_ssbo(tmp_str, byte_count);
  }

  if (!cascade->cpu_colors) {
    uint64_t byte_count = brick_count * sizeof(BrickColor);
    total_bytes += byte_count;
    cascade->cpu_colors = (BrickColor *)calloc(byte_count, 1);
    sprintf(tmp_str, "cascade-level-%u-colors", level);
    cascade->gpu_colors = rawkit_gpu_ssbo(tmp_str, byte_count);
  }
  printf("init cascade #%u: %u^3 (%llu bricks) (%llu bytes)\n", level, CASCADE_DIAMETER, brick_count, total_bytes);
}

void cascade_rebuild_worker(Cascade *cascade, state_t *state, vec3 center) {
  Prof p("cascade");
  float d = CASCADE_DIAMETER * 2.0f;
  float h = d * 0.5f;

  uint32_t count = 0;
  for (float x=0.0f; x<d; x++) {
    for (float z=0.0f; z<d; z++) {
      double depth = 0.2f;

      // TODO: y is actually up :facepalm:
      float bx = floor(x - center.x - h);
      float bz = floor(z - center.z - h);

      depth = floor(perlin2d(
        bx,
        bz,
        0.05f,
        2
      ) * 20.0f);

      for (float y=depth-2.0f; y<depth; y++) {
        uint32_t brick_idx = count++;
        Brick *brick = &cascade->cpu_bricks[brick_idx];
        brick->pos.x = bx;
        brick->pos.y = y;
        brick->pos.z = bz;

        BrickColor *brick_color = &cascade->cpu_colors[brick_idx];

        for (int cx=0; cx<8; cx++) {
          for (int cy=0; cy<8; cy++) {
            for (int cz=0; cz<8; cz++) {
              uint32_t c = (
                static_cast<uint8_t>(127.0f + (float)cx/8.0f * 127.0f) << 24 |
                static_cast<uint8_t>(127.0f + (float)cy/8.0f * 127.0f) << 16 |
                static_cast<uint8_t>(127.0f + (float)cz/8.0f * 127.0f) << 8  |
                0xFF
              );
              brick_color->voxel[cx + cy * 8 + cz * 8 * 8] = c;
            }
          }
        }
      }
    }
  }

  cascade->count = count;
  printf("\nmarking rebuild complete\n");
  cascade->rebuild_complete.store((int)RebuildState::COMPLETE);
  printf("marked rebuild complete\n");
}

void cascade_rebuild(state_t *state, Cascade *cascade, vec3 center) {
  int none = (int)RebuildState::NONE;
  int active = (int)RebuildState::ACTIVE;
  int complete = (int)RebuildState::COMPLETE;

  if (cascade->rebuild_complete.compare_exchange_strong(complete, none)) {
    printf("\nupload cascade data\n");
    // update brick positions
    {
      rawkit_gpu_ssbo_transition(cascade->gpu_bricks, (
        VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_SHADER_WRITE_BIT
      ));


      VkResult err = rawkit_gpu_ssbo_update(
        cascade->gpu_bricks,
        rawkit_vulkan_queue(),
        rawkit_vulkan_command_pool(),
        (void *)cascade->cpu_bricks,
        cascade->count * sizeof(Brick)
      );

      cascade->gpu_bricks->resource_version++;

      rawkit_gpu_ssbo_transition(cascade->gpu_bricks, (
        VK_ACCESS_SHADER_READ_BIT
      ));

      state->scene.cascade_center = vec4(center, 1.0);
    }

    // update brick colors
    {
      rawkit_gpu_ssbo_transition(cascade->gpu_colors, (
        VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_SHADER_WRITE_BIT
      ));


      VkResult err = rawkit_gpu_ssbo_update(
        cascade->gpu_colors,
        rawkit_vulkan_queue(),
        rawkit_vulkan_command_pool(),
        (void *)cascade->cpu_colors,
        cascade->count * sizeof(BrickColor)
      );

      cascade->gpu_colors->resource_version++;

      rawkit_gpu_ssbo_transition(cascade->gpu_colors, (
        VK_ACCESS_SHADER_READ_BIT
      ));
    }
    return;
  }

  if (cascade->count && all(equal(vec3(state->scene.cascade_center), center))) {
  // (cascade->camera.pos * INV_CASCADE_DIAMETER), floor(state->camera.pos * INV_CASCADE_DIAMETER)))) {
    return;
  }

  if (cascade->rebuild_complete.compare_exchange_strong(none, active)) {
    printf("\nkick off a rebuild..\n");
    cascade->camera = state->camera;

    if (cascade->pending.joinable()) {
      printf("pending join\n");
      cascade->pending.join();
      printf("pending joined\n");
    }
    cascade->pending = thread(cascade_rebuild_worker, cascade, state, state->scene.cascade_center);
  }
}


double lastTime = 0.0;
const rawkit_texture_sampler_t *nearest_sampler = NULL;


void worker(int i) {
  printf("WORKER: %i\n", i);

}

void setup() {
  printf("rebuild\n");

  std::thread t2(worker, 1); // pass by value
  t2.join();
  state_t *state = rawkit_hot_state(__FILE__, state_t);

  for (uint32_t i=0; i<CASCADE_COUNT; i++) {
    cascade_init(&state->cascades[i], i);
  }

  lastTime = rawkit_now();

  nearest_sampler = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_NEAREST,
    VK_FILTER_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
    0.0f,
    false,
    0.0f,
    false,
    VK_COMPARE_OP_NEVER,
    0,
    1,
    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    false
  );
}

void loop() {
  state_t *state = rawkit_hot_state(__FILE__, state_t);

  double now = rawkit_now();
  double dt = now - lastTime;
  lastTime = now;


  igShowDemoWindow(0);

  rawkit_vg_t *vg = rawkit_default_vg();
  Camera *camera = &state->camera;
  vec2 window(
    (float)rawkit_window_width(),
    (float)rawkit_window_height()
  );
  rawkit_vg_translate(vg, window.x/2.0f, window.y/2.0f);
  // camera movement
  {
    double move = dt * 100.0;

    // w
    if (igIsKeyDown(87)) {
      camera->pos.z += move;
    }

    // a
    if (igIsKeyDown(65)) {
      camera->pos.x -= move;
    }

    // s
    if (igIsKeyDown(83)) {
      camera->pos.z -= move;
    }

    // d
    if (igIsKeyDown(68)) {
      camera->pos.x += move;
    }

    // space
    if (igIsKeyDown(341)) {
      camera->pos.y -= move;
    }

    // control
    if (igIsKeyDown(32)) {
      camera->pos.y += move;
    }

    igText("camera(%f.03, ..., %f.03)", camera->pos.x, camera->pos.z);
  }

  // setup state
  {
    state->scene.screen_dims = vec4(
      (float)rawkit_window_width(),
      (float)rawkit_window_height(),
      0,
      0
    );

    mat4 proj = glm::perspective(
      glm::radians(90.0f),
      state->scene.screen_dims.x / state->scene.screen_dims.y,
      0.1f,
      10000.0f
    );


    // vec3 relative = vec3(state->scene.cascade_center - camera->pos);
    // relative.y = 20.0f;
    // igText("relative(%.01f, %.01f, %.01f)", relative.x, relative.y, relative.z);
    vec3 relative(
      camera->pos.x,
      camera->pos.y,
      camera->pos.z
    );
    mat4 view = glm::lookAt(
      relative,
      relative + vec3(0.0, -10.75, 19.0),
      vec3(0.0f, -1.0f, 0.0f)
    );

    state->scene.worldToScreen = proj * view;
    state->scene.brick_dims = vec4(16.0f);
    state->scene.eye = vec4(relative, 1.0f);
    // state->scene.eye = vec4(eye, 1.0f);
    state->scene.time = (float)rawkit_now();

  }

  // render 2d cascade
  {

    float cascade_step_size = 1.0f;
    vec3 cascade_center = vec3(ceil(state->camera.pos / cascade_step_size) * cascade_step_size);
    igText("center(%0.1f, %0.1f, %0.1f)",
      cascade_center.x,
      cascade_center.y,
      cascade_center.z
    );

    Cascade *cascade = &state->cascades[0];

    // update the cascade with dummy data for now
    cascade_rebuild(state, cascade, cascade_center);

    // noise texture debug
    if (0) {
      rawkit_texture_t *tex = rawkit_texture_mem(
        "cascade-texture",
        CASCADE_DIAMETER,
        CASCADE_DIAMETER,
        1,
        VK_FORMAT_R32G32B32A32_SFLOAT
      );
      tex->resource_version = 1;

      // clear the output texture
      {
        rawkit_shader_t *shader = rawkit_shader(
          rawkit_file("shader/clear-image.comp")
        );
        rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
          rawkit_default_gpu(),
          shader,
          NULL,
          0
        );

        if (inst) {
          rawkit_shader_instance_param_texture(inst, "tex", tex, NULL);
          rawkit_shader_instance_dispatch_compute(
            inst,
            CASCADE_DIAMETER,
            CASCADE_DIAMETER,
            1
          );
          rawkit_shader_instance_end(inst);
        }
      }

      // raster the cascade to the ouput texture
      {
        rawkit_shader_t *raster_cascade = rawkit_shader(
          rawkit_file("shader/cascade.comp")
        );
        rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
          rawkit_default_gpu(),
          raster_cascade,
          NULL,
          0
        );

        if (inst) {
          // rawkit_shader_instance_param_ubo(inst, "UBO", &world_ubo);
          rawkit_shader_instance_param_texture(inst, "tex", tex, NULL);
          rawkit_shader_instance_param_ssbo(inst, "Bricks", cascade->gpu_bricks);

          rawkit_shader_instance_dispatch_compute(
            inst,
            cascade->count,
            1,
            1
          );
          rawkit_shader_instance_end(inst);
        }
      }

      // draw the output texture
      {
        NVGpaint cascade_paint = rawkit_vg_texture(
          vg,
          0.0f,
          0.0f,
          64.0f,
          64.0f,
          0.0f,

          tex,
          1.0f,
          nearest_sampler
        );

        rawkit_vg_save(vg);
          rawkit_vg_scale(vg, 4.0, 4.0);
          rawkit_vg_begin_path(vg);
            rawkit_vg_draw_texture_rect(
              vg,
              0,
              0,
              CASCADE_DIAMETER,
              CASCADE_DIAMETER,
              -CASCADE_DIAMETER*0.5f,
              -CASCADE_DIAMETER*0.5f,
              tex,
              nearest_sampler
            );

          rawkit_vg_begin_path(vg);
            rawkit_vg_stroke_width(vg, 0.05f);
            rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));
            rawkit_vg_rect(vg, -CASCADE_DIAMETER*0.5f, -CASCADE_DIAMETER*0.5f, CASCADE_DIAMETER, CASCADE_DIAMETER);
            rawkit_vg_stroke(vg);
        rawkit_vg_restore(vg);


      }
    }

    // draw the bricks as cubes
    {
      Cascade *cascade = &state->cascades[0];
      rawkit_shader_t *shader = rawkit_shader(
        rawkit_file("shader/cascade-brick.vert"),
        rawkit_file("shader/cascade-brick.frag")
      );

      rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);
      if (inst) {
        rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
        rawkit_shader_instance_param_ssbo(inst, "Bricks", cascade->gpu_bricks);
        rawkit_shader_instance_param_ssbo(inst, "BrickColors", cascade->gpu_colors);

        VkViewport viewport = {
          .x = 0.0f,
          .y = 0.0f,
          .width = state->scene.screen_dims.x,
          .height = state->scene.screen_dims.y,
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
        igText("rendering bricks: %u", cascade->count);
        vkCmdDraw(inst->command_buffer, 36, cascade->count, 0, 0);
        rawkit_shader_instance_end(inst);
      }
    }

  }
}

