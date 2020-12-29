/*
  sdf.cpp - a quick bricks2 experiment testing if rendering bounded sdf ops with bricks
  is efficient enough without maintaining a separate spacial datastructure.

  STATUS:
    Currently computing a single object (a grid of small spheres) and rendering each touched brick
    with _all_ of the ops in the object (1024 ops as of this writing).

    From what I can tell, the shader is stalling due to cache thrashing and the branches for each type
    of op. Reducing the memory accesses and the branches equates to a ~2x jump in fps (15-30fps).

  RESULT
    With this approach we go back to the many pixels per voxel raytracing situation and add the extra
    overhead of reading a large list of ops per ray step.

    I can't seem to wrap my head around how this would work for dynamic modification. One of the goals for me
    is a cnc milling simulation and with this approach I'd need to apply _many_ swept cylinders.

  FUTURE
    If work on this continues, there are several paths of exploration:
    - instead of a list of ops per brick, denote what a brick is and render lists of bricks based on their type
    - precompute lists of surface bricks with their contents filled.

    To improve perf without adding a bunch of extra machinery + structure:
    - more culling of bricks + tigher aabbs
    - tune brick size
    - compute op list at a brick level instead of having each brick read every op in the object
*/



#include <rawkit/rawkit.h>

#define CPU_HOST
#include "bricks/shared.h"
#include "bricks/aabb.h"
#include "bezier.h"
#include <stb_sb.h>


#include <unordered_map>
#include <string>
using namespace std;

float brick_diameter = 8.0f;

AABB op_aabb(op_t op) {
  AABB ret = {
    .lb = vec3(0.0),
    .ub = vec3(0.0),
  };

  ret = aabb_add(ret, vec3(op.a), op.a.w);
  ret = aabb_add(ret, vec3(op.b), op.b.w);
  ret = aabb_add(ret, vec3(op.c), op.c.w);
  ret = aabb_add(ret, vec3(op.d), op.d.w);

  return ret;
}

void aabb_print(const char *desc, AABB aabb) {
  printf("aabb: %s: (%0.1f, %0.1f, %0.1f) -> (%0.1f, %0.1f, %0.1f)\n",
    desc,
    aabb.lb.x,
    aabb.lb.y,
    aabb.lb.z,
    aabb.ub.x,
    aabb.ub.y,
    aabb.ub.z
  );
}

struct OpList {
  op_t *ops = NULL;
  AABB *aabbs = NULL;
  rawkit_gpu_ssbo_t *ssbo = NULL;
  AABB aabb;
  void reset() {
    if (this->ops) {
      stb__sbn(this->ops) = 0;
    }

    if (this->aabbs) {
      stb__sbn(this->aabbs) = 0;
    }

    this->aabb.lb = vec3(0.0);
    this->aabb.ub = vec3(0.0);
  }

  int32_t bezier(vec4 c0, vec4 c1, vec4 c2, vec4 c3) {
    op_t op = {
      .control = { OP_ID_BEZIER, 0.0, 0.0, 0.0},
      .a = c0,
      .b = c1,
      .c = c2,
      .d = c3,
    };

    int32_t l = sb_count(this->ops);

    float max_r = glm::max(c0.w, glm::max(c1.w, glm::max(c2.w, c3.w)));

    // AABB op_bounds = bezier_aabb(
    //   vec3(c0),
    //   vec3(c1),
    //   vec3(c2),
    //   vec3(c3)
    // );

    // op_bounds.lb -= vec3(max_r);
    // op_bounds.ub += vec3(max_r);

    AABB op_bounds = {};
    op_bounds = aabb_add(op_bounds, vec3(c0), c0.w);
    op_bounds = aabb_add(op_bounds, vec3(c1), c1.w);
    op_bounds = aabb_add(op_bounds, vec3(c2), c2.w);
    op_bounds = aabb_add(op_bounds, vec3(c3), c3.w);

    // TODO: tigher bounds
    this->aabb = aabb_union(this->aabb, op_bounds);

    sb_push(this->ops, op);
    sb_push(this->aabbs, op_bounds);
    return (int32_t)l;
  }

  int32_t sphere(vec3 pos, float radius) {
    op_t op = {
      .control = vec4(OP_ID_SPHERE, 0.0, 0.0, 0.0),
      .a = vec4(pos.x, pos.y, pos.z, radius),
    };

    int32_t l = sb_count(this->ops);
    AABB op_bounds = {
      .lb = pos - radius,
      .ub = pos + radius,
    };

    // TODO: tigher bounds
    this->aabb = aabb_union(this->aabb, op_bounds);



    sb_push(this->ops, op);
    sb_push(this->aabbs, op_bounds);
    return (int32_t)l;
  }
};

char tmp_str[512] = "\0";

struct Object {
  string name = "<null>";
  Brick *bricks = nullptr;
  OpList ops;
  rawkit_gpu_ssbo_t *ssbo;

  void upload() {

    // upload ops
    uint32_t op_count = sb_count(this->ops.ops);
    {
      uint64_t size = sizeof(op_t) * op_count;
      string name = this->name + "::ops";
      sprintf(tmp_str, "%s::ops", this->name.c_str());
      this->ops.ssbo = rawkit_gpu_ssbo(tmp_str, size);
      rawkit_gpu_ssbo_update(
        this->ops.ssbo,
        rawkit_vulkan_queue(),
        rawkit_vulkan_command_pool(),
        (void *)this->ops.ops,
        size
      );

      this->ops.ssbo->resource_version++;
    }

    // recompute bricks
    {
      if (this->bricks) {
        stb__sbn(this->bricks) = 0;
      }

      // convert these to brick space
      vec3 lb = sign(ops.aabb.lb) * ceil(abs(ops.aabb.lb / brick_diameter)) * brick_diameter;
      vec3 ub = sign(ops.aabb.ub) * ceil(abs(ops.aabb.ub / brick_diameter)) * brick_diameter;
      for (float x=lb.x; x<ub.x; x+=brick_diameter) {
        for (float y=lb.y; y<ub.y; y+=brick_diameter) {
          for (float z=lb.z; z<ub.z; z+=brick_diameter) {

            bool contained = false;
            for (uint32_t op_idx=0; op_idx < op_count && !contained; op_idx++) {
              contained = aabb_contains(this->ops.aabbs[op_idx], vec3(x, y, z), brick_diameter);
              // TODO: sample the distance field and compare it to the circumscribed circle of this brick
            }

            if (!contained) {
              continue;
            }

            Brick brick = {
              .pos = floor(vec4(x / brick_diameter, y / brick_diameter, z / brick_diameter, brick_diameter)),
              .params = uvec4(op_count),
            };
            sb_push(this->bricks, brick);
          }
        }
      }
    }

    // upload bricks
    {
      uint64_t count = sb_count(this->bricks);
      sprintf(tmp_str, "%s::bricks", this->name.c_str());
      uint64_t size = sizeof(Brick) * count;
      this->ssbo = rawkit_gpu_ssbo(tmp_str, size);
      rawkit_gpu_ssbo_update(
        this->ssbo,
        rawkit_vulkan_queue(),
        rawkit_vulkan_command_pool(),
        (void *)this->bricks,
        size
      );
      this->ssbo->resource_version++;
    }
  }

  void render(rawkit_shader_instance_t *inst) {
    uint32_t instances = sb_count(this->bricks);
    if (!instances) {
      return;
    }

    igText("drawing %u bricks (ops=%u)", instances, sb_count(this->ops.ops));

    rawkit_shader_instance_param_ssbo(inst, "Bricks", this->ssbo);

    rawkit_shader_instance_param_ssbo(inst, "Ops", this->ops.ssbo);

    vkCmdDraw(inst->command_buffer, 36, instances, 0, 0);
  }
};

struct State {
  Object **objects;
  Scene scene;
};


bool rebuild = false;
void setup() {
  rebuild = true;

  State *state = rawkit_hot_state("state", State);
  if (!state->objects) {
    Object *obj = new Object;
    obj->name.assign("test cube");
    sb_push(state->objects, obj);
  }

}
void loop() {
  State *state = rawkit_hot_state("state", State);
  if (!state) {
    printf("ERROR: could not allocate space for state\n");
    return;
  }

  float scene_diameter = 1.0f;
  float now = (float)rawkit_now() * .15 + 5.0;

  if (true || state && state->objects && rebuild) {
    rebuild = false;

    Object *obj = state->objects[0];
    obj->ops.reset();

    // for (float x=0.0; x<64.0f; x+=64.0f) {
    //   for (float z=0.0; z<64.0f; z+=64.0f) {

    //     obj->ops.bezier(
    //       vec4(x, 0.0, z, 5.0),
    //       vec4(x + 32, 255.0, z + abs(sin(now) * 150.0), 40.0),
    //       vec4(x + 32.0, 400.0, z, 10.0),
    //       vec4(x + sinf(now * 10.0) * (50.0), -100.0 + sin(now * 10.0) * 10.0, 0.0, 10.0f + abs(cos(now) * 50))
    //     );
    //   }
    // }
    for (float x=0.0; x<128.0f; x+=8.0f) {
      for (float y=0.0; y<32.0f; y+=8.0f) {
        for (float z=0.0; z<128.0f; z+=8.0f) {
          obj->ops.sphere(vec3(x, y, z), 4.0f);
        }
      }
    }
    obj->upload();
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

    vec3 center = vec3(scene_diameter * 0.5f) + 0.5f;


    float dist = length(center) * 5.0;

    vec3 eye = center + vec3(
      sin(now) * dist,
      dist * 1.5,
      cos(now) * dist
    );

    // eye = vec3(dist, dist, dist);

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

  // render the world
  {
    rawkit_shader_t *world_shader = rawkit_shader(
      rawkit_file("bricks/brick.vert"),
      rawkit_file("bricks/lpos.frag")
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

      uint32_t l = sb_count(state->objects);
      for (uint32_t i=0; i<l; i++) {
        Object *o = state->objects[i];
        if (!o) {
          continue;
        }

        o->render(inst);
      }

      rawkit_shader_instance_end(inst);
    }
  }

}