#include <rawkit/rawkit.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/compatibility.hpp"
#include "glm/glm.hpp"
using namespace glm;


#include "perlin.h"

struct Brick {
  // vec3 pos;
  float x;
  float y;
  float z;
  float w;
  // uint64_t occlusion[64];
};


struct Cascade {
  uint32_t level;
  uint32_t count;
  Brick *cpu_bricks;
  rawkit_gpu_ssbo_t *gpu_bricks;
};

struct Camera {
  vec4 pos;
};

#define CASCADE_COUNT 1
#define CASCADE_DIAMETER 96
struct state_t {
  Cascade cascades[1];
  Camera camera;
};


char tmp_str[512] = "\0";

void cascade_init(Cascade *cascade, uint32_t level) {
  uint64_t brick_count = pow(CASCADE_DIAMETER, 3);
  uint64_t byte_count = brick_count * sizeof(Brick);
  printf("init cascade #%u: %u^3 (%llu bricks) (%llu bytes)\n", level, CASCADE_DIAMETER, brick_count, byte_count);
  if (!cascade->cpu_bricks) {
    cascade->cpu_bricks = (Brick *)calloc(byte_count, 1);
  }

  sprintf(tmp_str, "cascade-level-%u", level);
  cascade->gpu_bricks = rawkit_gpu_ssbo(tmp_str, byte_count);
}

double lastTime = 0.0;
const rawkit_texture_sampler_t *nearest_sampler = NULL;
void setup() {
  printf("rebuild\n");

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

struct Prof {
  const char *name;
  double start = 0.0;
  Prof(const char *name)
    : name(name)
  {
    this->start = rawkit_now();
  }

  ~Prof() {
    igText("%s took %f ms", this->name, (rawkit_now() - this->start) * 1000.0);
  }
};

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
      camera->pos.y -= move;
    }

    // a
    if (igIsKeyDown(65)) {
      camera->pos.x -= move;
    }

    // s
    if (igIsKeyDown(83)) {
      camera->pos.y += move;
    }

    // d
    if (igIsKeyDown(68)) {
      camera->pos.x += move;
    }

    igText("camera(%f.03, %f.03)", camera->pos.x, camera->pos.y);
  }

  // render 2d cascade
  {

    Cascade *cascade = &state->cascades[0];
    // update the cascade with dummy data for now
    {
      Prof p("cascade");
      float h = CASCADE_DIAMETER * 0.5f;
      cascade->count=0;
      for (float x=0.0f; x<CASCADE_DIAMETER; x++) {
        for (float y=0.0f; y<CASCADE_DIAMETER; y++) {
          double depth = 1.0f;

          depth = perlin2d(
            x + (camera->pos.x - h),
            y + (camera->pos.y - h),
            0.1f,
            7
          );

          for (float z=0.0f; z<CASCADE_DIAMETER; z++) {
            if (depth > 0.15) {
              Brick *brick = &cascade->cpu_bricks[cascade->count++];
              brick->x = x;
              brick->y = y;
              brick->z = depth;
            }
          }
        }
      }
    }

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
    }

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
        rawkit_vg_scale(vg, 16.0, 16.0);
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
}

