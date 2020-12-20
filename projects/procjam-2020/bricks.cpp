#include <rawkit/rawkit.h>
#define CPU_HOST
#include "bricks/shared.h"
#include <stb_sb.h>

#include <unordered_map>
#include <string>
using namespace std;


struct AABB {
  vec3 lb;
  vec3 ub;
  void add(vec3 p) {
    this->lb = glm::min(p, this->lb);
    this->ub = glm::max(p + vec3(1.0), this->lb);
  }
};

char tmp_str[512] = "\0";

struct Object {
  string name;
  vec4 pos;
  quat rot;
  AABB aabb = {.lb = vec3(0.0), .ub = vec3(0.0) };
  Brick *bricks = NULL;

  rawkit_gpu_ssbo_t *ssbo = NULL;

  void upload() {
    uint64_t count = sb_count(this->bricks);
    printf("uploading '%s' bricks (%u)\n", this->name.c_str(), count);
    uint64_t size = sizeof(Brick) * count;
    this->ssbo = rawkit_gpu_ssbo(this->name.c_str(), size);
    rawkit_gpu_ssbo_update(
      this->ssbo,
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      (void *)this->bricks,
      size
    );
    this->ssbo->resource_version++;
  }

  void add_brick(Brick brick) {
    this->aabb.add(brick.pos);
    sb_push(this->bricks, brick);
  }

  void render(rawkit_shader_instance_t *inst) {
    uint32_t instances = sb_count(this->bricks);
    igText("drawing %u instances", instances);
    rawkit_shader_instance_param_ssbo(inst, "Bricks", this->ssbo);
    vkCmdDraw(inst->command_buffer, 36, instances, 0, 0);
  }

};

struct World {
  Object **objects = NULL;
};

struct State {
  World world;
  Scene scene;
};



void setup() {

  State *state = rawkit_hot_state("state", State);

  if (true || !state->world.objects) {
    if (state->world.objects) {
      stb__sbn(state->world.objects[0]->bricks) = 0;
      stb__sbn(state->world.objects) = 0;
    }

    uint32_t object_id = 0;
    for (float y=0.0; y<10.0f; y+=2) {
      Object *obj = new Object;
      sprintf(tmp_str, "object#%u", object_id++);
      obj->name.assign(tmp_str);

      for (float x = 0.0f; x<8.0f; x++) {
        for (float z = 0.0f; z<8.0f; z++) {
         //memset(&brick.occlusion, 0xFF, sizeof(brick.occlusion));
          obj->add_brick({
            .pos = vec4(x, y, z, 0.0)
          });
        }
      }
      sb_push(state->world.objects, obj);
      obj->upload();
    }
  }
}

void loop() {

  State *state = rawkit_hot_state("state", State);
  uint32_t obj_count = sb_count(state->world.objects);

  // debug
  if (0) {
    uint32_t object_count = sb_count(state->world.objects);
    igText("objects: %u", object_count);
    for (uint32_t i=0; i<object_count; i++) {
      Object *obj = state->world.objects[i];
      uint32_t brick_count = sb_count(obj->bricks);
      igText("  %s; %u bricks\n", obj->name.c_str(), brick_count);
      for (uint32_t brick_idx = 0; brick_idx<brick_count; brick_idx++) {
        Brick *b = &obj->bricks[brick_idx];
        igText("    (%f, %f, %f)", b->pos.x, b->pos.y, b->pos.z);
      }
    }
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

    vec3 center = obj_count > 0
      ? (state->world.objects[0]->aabb.ub - state->world.objects[0]->aabb.lb)/2.0f
      : vec3(1.0);


    float dist = length(center) * 2.0;
    float now = (float)rawkit_now() * .5 + 5.0;
    vec3 eye = center + vec3(
      sin(now) * dist,
      dist * 0.125,
      cos(now) * dist
    );

    mat4 view = glm::lookAt(
      eye,
      center,
      vec3(0.0f, -1.0f, 0.0f)
    );

    state->scene.worldToScreen = proj * view;
    state->scene.brick_dims = vec4(16.0f);
    state->scene.eye = vec4(eye, 1.0f);
    state->scene.time = (float)rawkit_now();
  }

  rawkit_shader_t *world_shader = rawkit_shader(
    rawkit_file("bricks/brick.vert"),
    rawkit_file("bricks/brick.frag")
  );
  // render the world shader

  for (uint32_t obj_idx=0; obj_idx<obj_count; obj_idx++) {
    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(world_shader);
    if (inst) {
      igText("rendering inst: %llu\n", inst->resource_id);
      rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);

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

      Object *obj = state->world.objects[obj_idx];
      obj->render(inst);

      rawkit_shader_instance_end(inst);
    }
  }


}