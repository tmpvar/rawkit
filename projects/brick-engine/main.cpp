#include <rawkit/rawkit.h>
#include <GLFW/glfw3.h>
#define CPU_HOST
#include "shared.h"
#include <stb_sb.h>
#include "camera.hpp"
#include <unordered_map>
#include <string>
#include <functional>
using namespace std;

#define ACTIVE_BRICK_COUNT 3000000
#define CUBE_INDICES_COUNT 18
#define CUBE_VERTICES_COUNT 8
#define INDICES_SIZE CUBE_INDICES_COUNT * ACTIVE_BRICK_COUNT * sizeof(uint32_t)


void sledgehammer_buffer_sync(rawkit_gpu_buffer_t *buffer, VkCommandBuffer command_buffer) {
  return;
  VkBufferMemoryBarrier memoryBarrier = {
    .srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                    VK_ACCESS_INDEX_READ_BIT |
                    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                    VK_ACCESS_UNIFORM_READ_BIT |
                    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                    VK_ACCESS_SHADER_READ_BIT |
                    VK_ACCESS_SHADER_WRITE_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                    VK_ACCESS_TRANSFER_READ_BIT |
                    VK_ACCESS_TRANSFER_WRITE_BIT |
                    VK_ACCESS_HOST_READ_BIT |
                    VK_ACCESS_HOST_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                    VK_ACCESS_INDEX_READ_BIT |
                    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                    VK_ACCESS_UNIFORM_READ_BIT |
                    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                    VK_ACCESS_SHADER_READ_BIT |
                    VK_ACCESS_SHADER_WRITE_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                    VK_ACCESS_TRANSFER_READ_BIT |
                    VK_ACCESS_TRANSFER_WRITE_BIT |
                    VK_ACCESS_HOST_READ_BIT |
                    VK_ACCESS_HOST_WRITE_BIT
  };

  rawkit_gpu_buffer_transition(
    buffer,
    command_buffer,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // srcStageMask
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // dstStageMask
    memoryBarrier
  );
}

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
    sledgehammer_buffer_sync(this->ssbo->buffer, inst->command_buffer);
    rawkit_shader_instance_param_ssbo(inst, "Bricks", this->ssbo);
    vkCmdDraw(inst->command_buffer, 36, instances, 0, 0);
  }

  void collect_bricks(std::function<void(Brick *brick)> cb) {
    uint32_t count = sb_count(this->bricks);
    for (uint32_t i=0; i<count; i++) {
      cb(&this->bricks[i]);
    }
  }

};

VkResult rawkit_gpu_buffer_map(rawkit_gpu_t *gpu, rawkit_gpu_buffer_t *dst, VkDeviceSize offset, VkDeviceSize size, std::function<void(void *buf)> cb) {
  if (size >= dst->size) {
    size = VK_WHOLE_SIZE;
  }

  void *ptr;
  VkResult err = vkMapMemory(
    gpu->device,
    dst->memory,
    offset,
    size,
    0,
    &ptr
  );

  if (err) {
    printf("ERROR: unable to map memory (%i)\n", err);
    return err;
  }

  cb(ptr);

  // flush
  {
    VkMappedMemoryRange flush = {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .memory = dst->memory,
      .offset = offset,
      .size = size,
    };

    err = vkFlushMappedMemoryRanges(
      gpu->device,
      1,
      &flush
    );

    if (err) {
      printf("ERROR: unable to flush mapped memory ranges (%i)\n", err);
    }
  }

  vkUnmapMemory(gpu->device, dst->memory);

  return err;
}

struct World {
  Object **objects = NULL;

  uint32_t active_bricks = 0;
  rawkit_gpu_ssbo_t *brick_ssbo = NULL;
  rawkit_texture_t *culling_debug_tex;

  void render(rawkit_shader_instance_t *inst, rawkit_gpu_buffer_t *index_buffer) {
    if (!this->brick_ssbo) {
      this->brick_ssbo = rawkit_gpu_ssbo("frame bricks", ACTIVE_BRICK_COUNT * sizeof(Brick));
      this->brick_ssbo->resource_version++;

      // TODO: do this when we need to change the set of active bricks
      // fill active bricks buffer
      {
        this->active_bricks = 0;
        uint32_t obj_count = sb_count(this->objects);
        for (uint32_t obj_idx=0; obj_idx<obj_count; obj_idx++) {
          Object *obj = this->objects[obj_idx];
          obj->render(inst);


          rawkit_gpu_buffer_map(
            this->brick_ssbo->gpu,
            this->brick_ssbo->staging_buffer,
            0,
            this->brick_ssbo->staging_buffer->size,
            [obj, this](void *buf){
              Brick *dst = (Brick *)buf;

              obj->collect_bricks([dst, this](Brick *brick) {
                Brick *d = &dst[this->active_bricks++];
                memcpy(d, brick, sizeof(Brick));
              });
            }
          );

          rawkit_gpu_copy_buffer(
            this->brick_ssbo->gpu,
            rawkit_vulkan_queue(),
            rawkit_vulkan_command_pool(),
            this->brick_ssbo->staging_buffer,
            this->brick_ssbo->buffer,
            this->brick_ssbo->buffer->size
          );
        }
      }

      sledgehammer_buffer_sync(this->brick_ssbo->buffer, inst->command_buffer);
    }

    #if 0
    uint32_t obj_count = sb_count(this->objects);
    for (uint32_t obj_idx=0; obj_idx<obj_count; obj_idx++) {
      Object *obj = this->objects[obj_idx];
        obj->render(inst);
    }
    #else
      sledgehammer_buffer_sync(this->brick_ssbo->buffer, inst->command_buffer);
      sledgehammer_buffer_sync(index_buffer, inst->command_buffer);

      rawkit_shader_instance_param_ssbo(inst, "Bricks", this->brick_ssbo);

      vkCmdBindIndexBuffer(
        inst->command_buffer,
        index_buffer->handle,
        0,
        VK_INDEX_TYPE_UINT32
      );
    #endif
  }
};

struct Visibility {
  rawkit_gpu_ssbo_t *count = NULL;
  rawkit_gpu_ssbo_t *index = NULL;
};


struct State {
  World world;
  Scene scene;
  Camera *camera;
  double last_time;
  vec2 last_mouse_pos;
  bool camera_rotating;

  rawkit_gpu_buffer_t *index_buffer;
  Visibility visibility;

};

void fill_brick(Brick *brick) {
  // memset(&brick->occlusion, 0, sizeof(brick->occlusion));
  // for (int x=0; x<4; x++) {
  //   for (int y=0; y<4; y++) {
  //     for (int z=0; z<4; z++) {
  //       int loc = x + y * 4 + z * 4 * 4;
  //       // checkerboard
  //       // brick.occlusion[0] = 6148914691236517000;
  //       brick->occlusion[loc] = 1;
  //     }
  //   }
  // }
}

struct DepthPyramid {
  uint32_t mips;
  uvec2 dims;
  uint32_t diameter;
  rawkit_texture_t *texture = NULL;
  const char * name;
  ~DepthPyramid() {}
  DepthPyramid(rawkit_texture_t *depth_texture, const char *name, uint32_t diameter = 512, uint32_t mips = 7)
    : mips(mips), diameter(diameter), name(name)
  {

    // setup the depth pyramid texture
    {
      this->dims = uvec2(diameter * 3 / 2, diameter);

      this->texture = rawkit_texture_mem(
        this->name,
        this->dims.x,
        this->dims.y,
        1,
        VK_FORMAT_R32_SFLOAT
      );
    }

    DepthPyramidConstants constants = {
      .dimensions = this->dims,
      .mip = 0,
      .diameter = diameter,
    };

    // populate depth pyramid base using the incoming depth texture
    if (1) {
      rawkit_shader_t *shader = rawkit_shader(
        rawkit_file("shader/depth-pyramid-mip0.comp")
      );

      rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
        rawkit_default_gpu(),
        shader,
        NULL,
        0
      );
      if (!inst) {
        return;
      }

      // dispatch the shader
      {
        // push constants?
        rawkit_shader_instance_param_ubo(inst, "DepthPyramidUBO", &constants);
        rawkit_shader_instance_param_texture(inst, "src_tex", depth_texture, NULL);
        rawkit_shader_instance_param_texture(inst, "dst_tex", this->texture, NULL);
        rawkit_shader_instance_dispatch_compute(
          inst,
          diameter,
          diameter,
          1
        );
      }

      rawkit_shader_instance_end(inst);
    }

    // fill the remaining mips of the depth pyramid
    {
      rawkit_shader_t *shader = rawkit_shader(
        rawkit_file("shader/depth-pyramid-mipN.comp")
      );

      for (uint32_t mip=1; mip<this->mips; mip++) {
        rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
          rawkit_default_gpu(),
          shader,
          NULL,
          0
        );
        if (!inst) {
          return;
        }
        uint32_t mip2 = 1 << mip;
        constants.mip = mip - 1;

        // push constants?
        rawkit_shader_instance_param_ubo(inst, "DepthPyramidUBO", &constants);
        rawkit_shader_instance_param_texture(inst, "tex", this->texture, NULL);
        rawkit_shader_instance_dispatch_compute(
          inst,
          diameter / mip2,
          diameter / mip2,
          1
        );
        rawkit_shader_instance_end(inst);
      }

    }
  }

  Visibility compute_visibility(World *world, Scene *scene) {
    Visibility visibility = {};


    sprintf(tmp_str, "%s/visibility.count", this->name);
    visibility.count = rawkit_gpu_ssbo_ex(
      rawkit_default_gpu(),
      tmp_str,
      sizeof(DrawIndexedIndirectCommand),
      0,
      VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
    );

    sprintf(tmp_str, "%s/visibility.index", this->name);
    visibility.index = rawkit_gpu_ssbo(
      tmp_str,
      sizeof(uint) * ACTIVE_BRICK_COUNT
    );

    // populate depth pyramid base using the incoming depth texture
    {
      rawkit_shader_t *shader = rawkit_shader(
        rawkit_file("shader/culling.comp")
      );

      rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
        rawkit_default_gpu(),
        shader,
        NULL,
        0
      );
      if (!inst) {
        return visibility;
      }

      // clear the debug image
      {
        VkClearColorValue value = {0};

        VkImageSubresourceRange range = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        };

        vkCmdClearColorImage(
          inst->command_buffer,
          world->culling_debug_tex->image,
          VK_IMAGE_LAYOUT_GENERAL, //world->culling_debug_tex->image_layout,
          &value,
          1,
          &range
        );
      }

      DepthPyramidConstants constants = {
        .dimensions = this->dims,
        .mip = 0,
        .diameter = this->diameter,
      };


      // // transition the indirect count buffer
      // {
      //   VkBufferMemoryBarrier extend = {
      //     .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      //   };
      //   VkResult err = rawkit_gpu_buffer_transition(
      //     visibility.count->buffer,
      //     inst->command_buffer,
      //     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      //     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      //     extend
      //   );
      // }
      sledgehammer_buffer_sync(visibility.count->buffer, inst->command_buffer);
      vkCmdFillBuffer(
        inst->command_buffer,
        visibility.count->buffer->handle,
        0,
        visibility.count->buffer->size,
        0
      );
      sledgehammer_buffer_sync(visibility.count->buffer, inst->command_buffer);

      // // transition the indirect count buffer
      // {
      //   VkBufferMemoryBarrier extend = {
      //     .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      //   };
      //   VkResult err = rawkit_gpu_buffer_transition(
      //     visibility.count->buffer,
      //     inst->command_buffer,
      //     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      //     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      //     extend
      //   );
      // }

      // transition the visibility buffer
      {
        VkBufferMemoryBarrier extend = {
          .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        };
        VkResult err = rawkit_gpu_buffer_transition(
          visibility.index->buffer,
          inst->command_buffer,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          extend
        );
      }

      sledgehammer_buffer_sync(visibility.index->buffer, inst->command_buffer);
      sledgehammer_buffer_sync(visibility.count->buffer, inst->command_buffer);
      if (world->brick_ssbo) {
        sledgehammer_buffer_sync(world->brick_ssbo->buffer, inst->command_buffer);
      }

      rawkit_shader_instance_param_ubo(inst, "DepthPyramidUBO", &constants);
      rawkit_shader_instance_param_ubo(inst, "UBO", scene);
      rawkit_shader_instance_param_texture(inst, "tex", this->texture, NULL);
      rawkit_shader_instance_param_texture(inst, "debug_tex", world->culling_debug_tex, NULL);
      rawkit_shader_instance_param_ssbo(inst, "Bricks", world->brick_ssbo);
      rawkit_shader_instance_param_ssbo(inst, "Index", visibility.index);
      rawkit_shader_instance_param_ssbo(inst, "Count", visibility.count);

      rawkit_shader_instance_dispatch_compute(
        inst,
        world->active_bricks,
        1,
        1
      );

      rawkit_shader_instance_end(inst);
    }

    return visibility;
  }
};


void setup() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();

  State *state = rawkit_hot_state("state", State);

  if (!state->index_buffer) {
    uint32_t cube_indices[36] = {
      0, 2, 1, 2, 3, 1,
      5, 4, 1, 1, 4, 0,
      0, 4, 6, 0, 6, 2,
      6, 5, 7, 6, 4, 5,
      2, 6, 3, 6, 7, 3,
      7, 1, 3, 7, 5, 1,
    };

    state->index_buffer = rawkit_gpu_buffer_create(
      "index-buffer",
      gpu,
      INDICES_SIZE,
      (
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT
      ),
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    uint32_t *mem = (uint32_t *)malloc(INDICES_SIZE);
    uint32_t len = CUBE_INDICES_COUNT * ACTIVE_BRICK_COUNT;
    for (uint32_t i=0; i<len; i++) {
      uint32_t cube = i / CUBE_INDICES_COUNT;
      uint32_t index = i % CUBE_INDICES_COUNT;
      mem[i] = cube_indices[index] + cube * CUBE_VERTICES_COUNT;
    }

    rawkit_gpu_buffer_update(state->index_buffer, mem, INDICES_SIZE);

    free(mem);
  }

  if (!state->world.objects) {
    if (state->world.objects) {
      stb__sbn(state->world.objects[0]->bricks) = 0;
      stb__sbn(state->world.objects) = 0;
    }

    uint32_t object_id = 0;
    Object *obj = new Object;
    sprintf(tmp_str, "object#%u", object_id++);
    obj->name.assign(tmp_str);
    float d = 32.0f;
    float s = 1.0f;
    float diagonal_length = glm::length(vec3(1.0f));
#if 1
    for (float y=0.0; y<d*.5; y++) {
      for (float x = 0.0f; x<d; x++) {
        for (float z = 0.0f; z<d; z++) {

          Brick brick = {
            .pos = vec4(x*s, y*s, z*s, diagonal_length)
          };
          // fill_brick(&brick);
          obj->add_brick(brick);
        }
      }
    }
#else
    Brick brick = {
      .pos = vec4(64.0f, 0.0f, 0.0f, diagonal_length)
    };
    // fill_brick(&brick);
    obj->add_brick(brick);

#endif

    sb_push(state->world.objects, obj);
    obj->upload();
  }

  if (!state->camera) {

    vec3 center = sb_count(state->world.objects) > 0
      ? (state->world.objects[0]->aabb.ub - state->world.objects[0]->aabb.lb)/2.0f
      : vec3(1.0);
    state->camera = new Camera;
    state->camera->type = Camera::CameraType::firstperson;
    state->camera->flipY = true;
    state->camera->setTranslation(center);
    state->camera->setRotation(vec3(0.0, 130.0, 0.0));

  }

  if (state->last_time == 0.0) {
    state->last_time = rawkit_now();
  }
}


//Fast Quadric Proj: "GPU-Based Ray-Casting of Quadratic Surfaces" http://dl.acm.org/citation.cfm?id=2386396
vec3 quadricProj(vec3 osPosition, float voxelSize, mat4 objectToScreenMatrix, vec2 screenSize)
{
    const vec4 quadricMat = vec4(1.0, 1.0, 1.0, -1.0);
    float sphereRadius = voxelSize * 1.732051;
    vec4 sphereCenter = vec4(vec3(osPosition), 1.0);
    mat4 modelViewProj = transpose(objectToScreenMatrix);

    mat3x4 matT = mat3x4(
      mat3(
        vec3(modelViewProj[0]),
        vec3(modelViewProj[1]),
        vec3(modelViewProj[3])
      ) * sphereRadius
    );
    matT[0].w = dot(sphereCenter, modelViewProj[0]);
    matT[1].w = dot(sphereCenter, modelViewProj[1]);
    matT[2].w = dot(sphereCenter, modelViewProj[3]);

    mat3x4 matD = mat3x4(matT[0] * quadricMat, matT[1] * quadricMat, matT[2] * quadricMat);
    vec4 eqCoefs = vec4(
      dot(matD[0], matT[2]),
      dot(matD[1], matT[2]),
      dot(matD[0], matT[0]),
      dot(matD[1], matT[1])
    ) / dot(matD[2], matT[2]);

    vec4 outPosition = vec4(eqCoefs.x, eqCoefs.y, 0.0, 1.0);
    vec2 AABB = sqrt(
      vec2(eqCoefs) * vec2(eqCoefs) + vec2(eqCoefs.z, eqCoefs.w)
    );
    AABB *= screenSize;
    return vec3(
      vec2(outPosition),
      glm::max(AABB.x, AABB.y)
    );
}


void loop() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();
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

  double now = rawkit_now();
  double dt = now - state->last_time;
  state->last_time = now;
  igText("dt: %f", dt);


  // mouse look
  if (1) {
    GLFWwindow *window = rawkit_glfw_window();
    if (window) {
      igText("has window");
      ImGuiIO *io = igGetIO();
      if (io) {
        igText("has io");
        if (!io->WantCaptureMouse) {
          if (igIsMouseDown(ImGuiMouseButton_Left)) {
            if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
              glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
              if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
              }


              double mx, my;
              glfwGetCursorPos(window, &mx, &my);
              state->last_mouse_pos = vec2(mx, my);
            }
          } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
           }
        }
      }
    } else {
      igText("no window...");
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

    state->world.culling_debug_tex = rawkit_texture_mem(
      "culling_debug_tex",
      rawkit_window_width(),
      rawkit_window_height(),
      1,
      VK_FORMAT_R32_UINT
    );

    // camera movement
    {
      // double move = dt * 100.0;

      // w
      state->camera->keys.up = igIsKeyDown(87);

      // a
      state->camera->keys.left = igIsKeyDown(65);

      // s
      state->camera->keys.down = igIsKeyDown(83);

      // d
      state->camera->keys.right = igIsKeyDown(68);

      // // space
      // if (igIsKeyDown(341)) {
      //   camera->pos.y -= move;
      // }

      // // control
      // if (igIsKeyDown(32)) {
      //   camera->pos.y += move;
      // }
    }

    state->camera->setPerspective(
      90.0f,
      state->scene.screen_dims.x / state->scene.screen_dims.y,
      0.1f,
      (float)MAX_DEPTH
    );
    state->camera->update(dt);


    GLFWwindow *window = rawkit_glfw_window();
    if (window && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
      double mx, my;
      glfwGetCursorPos(window, &mx, &my);

      float sensitivity = 0.2;
      vec2 diff = -vec2(
        state->last_mouse_pos.x - mx,
        state->last_mouse_pos.y - my
      ) * sensitivity;

      igText("rotating (%f, %f)",
        diff.x,
        diff.y
      );
      state->camera->rotate(vec3(
        diff.y,
        diff.x,
        0.0
      ));

      vec2 screen_center = state->scene.screen_dims * 0.5f;
      state->last_mouse_pos = vec2(mx, my);
    }

    state->camera->setMovementSpeed(10.0);
    igText("camera pos(%f, %f, %f)\n       rot(%f, %f, %f)",
      state->camera->position.x, state->camera->position.y, state->camera->position.z,
      state->camera->rotation.x, state->camera->rotation.y, state->camera->rotation.z
    );
    mat4 clip = glm::mat4(
      1.0f,  0.0f, 0.0f, 0.0f,
      0.0f, -1.0f, 0.0f, 0.0f,
      0.0f,  0.0f, 0.5f, 0.0f,
      0.0f,  0.0f, 0.5f, 1.0f
    );
    state->scene.worldToScreen = clip * state->camera->matrices.perspective * state->camera->matrices.view;

    // proj[1][1] *= -1.0f;

    vec3 center = obj_count > 0
      ? (state->world.objects[0]->aabb.ub - state->world.objects[0]->aabb.lb)/2.0f
      : vec3(1.0);


    float dist = length(center) * .1;
    float now = (float)rawkit_now() * .5 + 5.0;
    // now = 1.5;
    // now = 3.14 * 3.0;
    // state->camera->position = center + vec3(
    //   sin(now) * dist,
    //   0.0,
    //   cos(now) * dist
    // );

    // mat4 view = glm::lookAt(
    //   state->camera->position,
    //   center,
    //   vec3(0.0f, 1.0f, 0.0f)
    // );

    // state->scene.worldToScreen = clip * state->camera->matrices.perspective * view;
    state->scene.brick_dims = vec4(64.0f);
    // state->scene.eye = vec4(state->camera->position, 1.0f);
    state->scene.eye = state->camera->viewPos;
    state->scene.time = vec4((float)rawkit_now());

    Brick *brick = &state->world.objects[0]->bricks[0];

    vec3 res = quadricProj(
      vec3(brick->pos),
      1.0f,
      state->scene.worldToScreen,
      vec2(state->scene.screen_dims)
    );

    igText("world pos(%f, %f, %f)", brick->pos.x, brick->pos.y, brick->pos.z);
    igText("time: %f", state->scene.time.x);
    vec4 p = state->scene.worldToScreen * vec4(vec3(brick->pos), 1.0f);
    igText("screen pos(%f, %f, %f) w=%f", p.x, p.y , p.z, p.w);
    igText("xform pos(%f, %f, %f) w=%f mip=%f", p.x / p.w, p.y / p.w, p.z / p.w, p.w, log2(p.z / p.w * 256.0f));
    igText("max bias pos(%f, %f, %f) mip=%f", p.x / p.w, p.y / p.w, (p.z + p.w) / 2.0f / MAX_DEPTH, log2((p.z + p.w) / 2.0f));

    res.x = res.x * 0.5f + 0.5f;
    res.y = res.y * 0.5f + 0.5f;

    igText("computedRadius: %f (%f, %f); mip: %f",
     res.z, res.x, res.y,
     log2(res.z)
    );


  }

  // render the world into a texture
  rawkit_texture_target_t *main_pass = rawkit_texture_target_begin(
    gpu,
    "main_pass",
    state->scene.screen_dims.x,
    state->scene.screen_dims.y,
    true
  );

  // render all bricks
  {
    bool indirect = state->visibility.count;
    const rawkit_file_t *files[2] = {
      indirect
        ? rawkit_file("shader/brick-indirect.vert")
        : rawkit_file("shader/brick.vert"),
      rawkit_file("shader/brick.frag")
    };

    rawkit_shader_t *world_shader = rawkit_shader_ex(
      gpu,
      main_pass->render_pass,
      nullptr,
      2,
      files
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
      gpu,
      world_shader,
      main_pass->command_buffer,
      rawkit_window_frame_index()
    );

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

      state->world.render(inst, state->index_buffer);

      if (!indirect) {
        vkCmdDrawIndexed(
          inst->command_buffer,
          state->world.active_bricks * 18, // index count
          1,                               // instance count
          0,                               // first index
          0,                               // vertex offset
          1                                // first instance
        );
      } else {

        // transition the indirect count buffer
        // {
        //   VkBufferMemoryBarrier extend = {
        //     .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        //     .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
        //   };
        //   VkResult err = rawkit_gpu_buffer_transition(
        //     state->visibility.count->buffer,
        //     inst->command_buffer,
        //     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        //     VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        //     extend
        //   );
        // }

        // transition the visibility buffer
        // {
        //   VkBufferMemoryBarrier extend = {
        //     .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        //   };
        //   VkResult err = rawkit_gpu_buffer_transition(
        //     state->visibility.index->buffer,
        //     inst->command_buffer,
        //     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        //     VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        //     extend
        //   );
        // }

        sledgehammer_buffer_sync(state->visibility.index->buffer, inst->command_buffer);
        sledgehammer_buffer_sync(state->visibility.count->buffer, inst->command_buffer);

        rawkit_shader_instance_param_ssbo(inst, "Index", state->visibility.index);
        vkCmdDrawIndexedIndirect(
          inst->command_buffer,
          state->visibility.count->buffer->handle,
          0, // offset
          1, // draw count
          0  // stride
        );
      }
      rawkit_shader_instance_end(inst);
    }

    rawkit_texture_target_end(main_pass);
  }

  DepthPyramid depth_pyramid(
    main_pass->depth,
    "main pass depth",
    512,
    7
  );

  igButton("no visibility", {150.0f, 20.0f});
  if (igIsItemActive()) {
    state->visibility.count = NULL;
  } else {

    igButton("pause visibility", {150.0f, 20.0f});
    igText("space: %i", igIsKeyDown(32));

    if (!state->visibility.count || (!igIsItemActive() && /* space */ !igIsKeyDown(32))) {
      state->visibility = depth_pyramid.compute_visibility(&state->world, &state->scene);
    }
  }

  {
    float scale = 0.5;
    ImTextureID texture = rawkit_imgui_texture(main_pass->color, main_pass->color->default_sampler);
    if (!texture) {
      return;
    }

    igImage(
      texture,
      (ImVec2){ 768.0f * scale, 512.0f * scale },
      (ImVec2){ 0.0f, 0.0f }, // uv0
      (ImVec2){ 1.0f, 1.0f }, // uv1
      (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
      (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
    );
  }

  float scale = 0.5;
  if (1) {
    {
      ImTextureID texture = rawkit_imgui_texture(depth_pyramid.texture, depth_pyramid.texture->default_sampler);
      if (!texture) {
        return;
      }

      igImage(
        texture,
        (ImVec2){ 768.0f * scale, 512.0f * scale },
        (ImVec2){ 0.0f, 0.0f }, // uv0
        (ImVec2){ 1.0f, 1.0f }, // uv1
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
      );
    }
    {
      ImTextureID texture = rawkit_imgui_texture(state->world.culling_debug_tex, state->world.culling_debug_tex->default_sampler);
      if (!texture) {
        return;
      }

      igImage(
        texture,
        (ImVec2){ 768.0f * scale, 512.0f * scale },
        (ImVec2){ 0.0f, 0.0f }, // uv0
        (ImVec2){ 1.0f, 1.0f }, // uv1
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
      );
    }
  }
  // fill screen with texture
  if (1) {
    rawkit_shader_t *shader = rawkit_shader(
      rawkit_file("shader/fullscreen.vert"),
      rawkit_file("shader/fullscreen.frag")
    );

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);
    if (inst) {
      rawkit_shader_instance_param_texture(inst, "tex_color", main_pass->color, NULL);
      rawkit_shader_instance_param_texture(inst, "culling_debug_tex", state->world.culling_debug_tex, NULL);

      VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)rawkit_window_width(),
        .height = (float)rawkit_window_height()
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
      vkCmdDraw(inst->command_buffer, 3, 1, 0, 0);
      rawkit_shader_instance_end(inst);
    }
  }
}
