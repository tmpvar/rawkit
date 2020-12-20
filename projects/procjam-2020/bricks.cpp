#include <rawkit/rawkit.h>
#define CPU_HOST
#include "bricks/shared.h"
#include <stb_sb.h>

#include <unordered_map>
#include <string>
using namespace std;


#define CASCADE_COUNT 2
#define CASCADE_DIAMETER 96


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



struct Cascade {
  rawkit_gpu_ssbo_t *brick_buffer = NULL;


  uint32_t *brick_indices = NULL;
  uint32_t level = 0;

  /*

    +-------------+
    | +---------+ |
    | | +-----+ | |
    | | |  o  | | |
    | | +-----+ | |
    | +---------+ |
    +-------------+

    level 0: level_diameter^3
    level 1: level_diameter^3^2

  */

  Cascade(uint32_t level): level(level) {
    // uint64_t size = instances * sizeof(Brick);
    // sprintf(tmp_str, "world cascade#%u", level);
    // this->ssbo = rawkit_gpu_ssbo(tmp_str, size);
  }

  void render(const Scene *scene, rawkit_shader_instance_t *inst) {
    uint64_t max_brick_count = pow(CASCADE_DIAMETER, 3);
    // TODO: max brick count is not
    igText("drawing %u bricks (level=%u)", max_brick_count, this->level);
    rawkit_shader_instance_param_ssbo(inst, "Bricks", this->brick_buffer);
    vkCmdDraw(inst->command_buffer, 36, max_brick_count, 0, 0);
  }
};



struct World {
  Object **objects = NULL;
  Cascade *cascades = NULL;
  Brick **bricks = NULL;

  void render(const Scene *scene, rawkit_shader_instance_t *inst) {
    uint32_t obj_count = sb_count(this->objects);
    for (uint32_t obj_idx=0; obj_idx<obj_count; obj_idx++) {
      Object *obj = this->objects[obj_idx];
      obj->render(inst);
    }

    // TODO: render cascades
    // uint32_t cascade_count = sb_count(this->cascades);
    // for (uint32_t cascade_idx=0; cascade_idx<CASCADE_COUNT; cascade_idx++) {
    //   Cascade *cascade = this->cascades[cascade_idx];
    //   if (!cascade) {
    //     continue;
    //   }
    //   cascade->render(scene, inst);
    // }
  }
};

struct State {
  World world;
  Scene scene;
};

void render_camera(Scene *scene) {
  rawkit_shader_t *shader = rawkit_shader(
    rawkit_file("bricks/camera.vert"),
    rawkit_file("bricks/camera.frag")
  );

  rawkit_mesh_t *mesh = rawkit_mesh("mesh/camera.stl");
  rawkit_gpu_vertex_buffer_t *vb = rawkit_gpu_vertex_buffer_create(
    rawkit_default_gpu(),
    rawkit_vulkan_queue(),
    rawkit_vulkan_command_pool(),
    mesh
  );

  rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);
  if (!inst || !inst->command_buffer || !vb->resource_version) {
    return;
  }

  rawkit_shader_instance_param_ubo(inst, "UBO", scene);

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = scene->screen_dims.x,
    .height = scene->screen_dims.y,
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

  VkDeviceSize offsets[1] = { 0 };
  vkCmdBindVertexBuffers(
    inst->command_buffer,
    0,
    1,
    &vb->vertices->handle,
    offsets
  );


  uint32_t index_count = rawkit_mesh_index_count(mesh);
  if (index_count > 0) {
    vkCmdBindIndexBuffer(
      inst->command_buffer,
      vb->indices->handle,
      0,
      VK_INDEX_TYPE_UINT32
    );

    vkCmdDrawIndexed(
      inst->command_buffer,
      index_count,
      1,
      0,
      0,
      0
    );
  } else {
    vkCmdDraw(
      inst->command_buffer,
      rawkit_mesh_vertex_count(mesh),
      1,
      0,
      0
    );
  }


  rawkit_shader_instance_end(inst);
}

void fill_brick(Brick *brick) {
  memset(&brick->occlusion, 0, sizeof(brick->occlusion));
  for (int x=0; x<4; x++) {
    for (int y=0; y<4; y++) {
      for (int z=0; z<4; z++) {
        int loc = x + y * 4 + z * 4 * 4;
        // checkerboard
        // brick.occlusion[0] = 6148914691236517000;
        brick->occlusion[loc] = 1;
      }
    }
  }
}

void setup() {

  State *state = rawkit_hot_state("state", State);

  if (!state->world.objects) {
    if (state->world.objects) {
      stb__sbn(state->world.objects[0]->bricks) = 0;
      stb__sbn(state->world.objects) = 0;
    }

    uint32_t object_id = 0;
    Object *obj = new Object;
    sprintf(tmp_str, "object#%u", object_id++);
    obj->name.assign(tmp_str);
    float d = 64.0f;
    for (float y=0.0; y<1.0; y++) {
      for (float x = 0.0f; x<d; x++) {
        for (float z = 0.0f; z<d; z++) {
          Brick brick = {
            .pos = vec4(x, y, z, 0.0)
          };
          // fill_brick(&brick);
          obj->add_brick(brick);
        }
      }
    }
    sb_push(state->world.objects, obj);
    obj->upload();
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
      1.1f,
      10000.0f
    );

    vec3 center = obj_count > 0
      ? (state->world.objects[0]->aabb.ub - state->world.objects[0]->aabb.lb)/2.0f
      : vec3(1.0);


    float dist = length(center) * .75;// * 1.5;
    float now = (float)rawkit_now() * .05 + 5.0;
    vec3 eye = center + vec3(
      sin(now) * dist,
      dist,
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

  // // render the world shader
  // {
  //   rawkit_shader_t *world_shader = rawkit_shader(
  //     rawkit_file("bricks/brick.vert"),
  //     rawkit_file("bricks/brick.frag")
  //   );

  //   for (uint32_t obj_idx=0; obj_idx<obj_count; obj_idx++) {
  //     rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(world_shader);
  //     if (inst) {
  //       rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);

  //       VkViewport viewport = {
  //         .x = 0.0f,
  //         .y = 0.0f,
  //         .width = state->scene.screen_dims.x,
  //         .height = state->scene.screen_dims.y,
  //         .minDepth = 0.0,
  //         .maxDepth = 1.0
  //       };

  //       vkCmdSetViewport(
  //         inst->command_buffer,
  //         0,
  //         1,
  //         &viewport
  //       );

  //       VkRect2D scissor = {};
  //       scissor.extent.width = viewport.width;
  //       scissor.extent.height = viewport.height;
  //       vkCmdSetScissor(
  //         inst->command_buffer,
  //         0,
  //         1,
  //         &scissor
  //       );

  //       Object *obj = state->world.objects[obj_idx];
  //       obj->render(inst);

  //       rawkit_shader_instance_end(inst);
  //     }
  //   }
  // }

  // render the world
  {
    rawkit_shader_t *world_shader = rawkit_shader(
      rawkit_file("bricks/brick.vert"),
      rawkit_file("bricks/brick.frag")
    );


    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(world_shader);
    if (inst) {
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
      state->world.render(&state->scene, inst);
      rawkit_shader_instance_end(inst);
    }
  }


  // render the camera
  {
    state->scene.camera.pos = vec4(10.0, 2.0, 10.0, 1.0);
    render_camera(&state->scene);
  }


}