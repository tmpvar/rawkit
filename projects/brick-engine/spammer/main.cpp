/*
 The brick spammer
 ~~~~~~~~~~~~~~~~~
 A fully dynamic world based on a sparse grid

 Bricks are world grid aligned AABBs
*/

#include <rawkit/rawkit.h>
#define CPU_HOST
#include "shared.h"
#include "camera.hpp"

#include "FastNoiseLite.h"

#include <functional>
using namespace std;

#define VISIBLE_GRID_DIMS uvec3(256, 128, 256)
// #define VISIBLE_GRID_DIMS uvec3(128, 64, 128)

// #define VISIBLE_GRID_DIMS uvec3(64, 32, 64)
#define BRICK_DIAMETER 32.0f // for an effective visible range of (4096, 2048, 4096)
#define MAX_BRICK_COUNT (VISIBLE_GRID_DIMS.x * VISIBLE_GRID_DIMS.y * VISIBLE_GRID_DIMS.z)
#define SPLAT_INDICES_COUNT 6
#define SPLAT_VERTICES_COUNT 4
#define INDICES_SIZE SPLAT_INDICES_COUNT * MAX_BRICK_COUNT * sizeof(u32)

struct State {
  rawkit_gpu_ssbo_t *positions;
  u32 *positions_staging;

  f32 *grid;

  rawkit_gpu_buffer_t *index_buffer;
  rawkit_gpu_ssbo_t *indirect_count;
  Scene scene;
  Camera *camera;

  u32 active_bricks;
  double last_time;
  vec2 last_mouse_pos;
  bool camera_rotating;
};

void setup() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();
  State *state = rawkit_hot_state("state", State);

  state->positions = rawkit_gpu_ssbo(
    "positions",
    MAX_BRICK_COUNT * sizeof(u32)
  );

  if (!state->positions_staging) {
    VkResult err = vkMapMemory(
      gpu->device,
      state->positions->staging_buffer->memory,
      0,
      state->positions->staging_buffer->size,
      0,
      (void **)&state->positions_staging
    );
  }

  if (!state->index_buffer) {
    u32 splat_indices[SPLAT_INDICES_COUNT] = {
      0, 2, 1, 2, 3, 1,
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

    u32 *mem = (u32 *)malloc(INDICES_SIZE);
    u32 len = SPLAT_INDICES_COUNT * MAX_BRICK_COUNT;
    for (uint32_t i=0; i<len; i++) {
      u32 splat = i / SPLAT_INDICES_COUNT;
      u32 index = i % SPLAT_INDICES_COUNT;
      mem[i] = splat_indices[index] + splat * SPLAT_VERTICES_COUNT;
    }

    rawkit_gpu_buffer_update(state->index_buffer, mem, INDICES_SIZE);

    free(mem);
  }

  // setup indirect count buffer
  {
    u64 size = sizeof(DrawIndexedIndirectCommand);
    state->indirect_count = rawkit_gpu_ssbo_ex(
      rawkit_default_gpu(),
      "indirect-count",
      size,
      0,
      VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
    );

    DrawIndexedIndirectCommand tmp = {
      .indexCount = 6,
      .instanceCount = 1,
    };
    rawkit_gpu_ssbo_update(
      state->indirect_count,
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      (void *)&tmp,
      size
    );
  }

  if (!state->camera) {
    state->camera = new Camera;
    state->camera->type = Camera::CameraType::firstperson;
    state->camera->flipY = true;
    state->camera->setTranslation(vec3(0));
    state->camera->setRotation(vec3(0.0, 0.0, 0.0));
  }

  if (state->last_time == 0.0) {
    state->last_time = rawkit_now();
  }

  if (!state->grid) {
    state->grid = (f32 *)calloc(MAX_BRICK_COUNT, sizeof(f32));
  }
}


void loop() {
  State *state = rawkit_hot_state("state", State);

  // frame setup
  {
    // mouse look
    {
      GLFWwindow *window = rawkit_glfw_window();
      if (window) {
        ImGuiIO *io = igGetIO();
        if (io) {
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
      }
    }

    // setup state
    {
      double now = rawkit_now();
      double dt = now - state->last_time;
      state->last_time = now;

      state->scene.screen_dims = vec4(
        (float)rawkit_window_width(),
        (float)rawkit_window_height(),
        0,
        0
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

        state->camera->setMovementSpeed(100.0);
      }

      state->camera->setPerspective(
        90.0f,
        state->scene.screen_dims.x / state->scene.screen_dims.y,
        0.5f,
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
      state->scene.eye = state->camera->viewPos;
      state->scene.time = f32(rawkit_now() * 0.1);
    }
  }

  // update the entire world
  {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    state->active_bricks = 0;
    for (u32 z=0; z<VISIBLE_GRID_DIMS.z; z++) {
      for (u32 y=0; y<VISIBLE_GRID_DIMS.y; y++) {
        for (u32 x=0; x<VISIBLE_GRID_DIMS.x; x++) {

          u32 idx = x + y * VISIBLE_GRID_DIMS.x + z * VISIBLE_GRID_DIMS.x * VISIBLE_GRID_DIMS.y;
          u32 pidx = x + 1 + y * VISIBLE_GRID_DIMS.x + z * VISIBLE_GRID_DIMS.x * VISIBLE_GRID_DIMS.y;
          state->grid[idx] = state->grid[pidx];
        }
      }
    }

    for (u32 z=0; z<VISIBLE_GRID_DIMS.z; z++) {
      for (u32 y=0; y<VISIBLE_GRID_DIMS.y; y++) {
        for (u32 x=0; x<VISIBLE_GRID_DIMS.x; x++) {

          u32 idx = x + y * VISIBLE_GRID_DIMS.x + z * VISIBLE_GRID_DIMS.x * VISIBLE_GRID_DIMS.y;

          if (x == VISIBLE_GRID_DIMS.x - 1) {
            f32 noise_value = noise.GetNoise(
              f64(x) * 100.0 + rawkit_now() * 100.0,
              f64(y),
              f64(z)
            );
            state->grid[idx] = noise_value;
          }

          f32 noise_value = state->grid[idx];
          if (noise_value < 0.2) {
            continue;
          }

          // state->positions_staging[state->active_bricks] = vec4(x, y, z, 0);
          state->positions_staging[state->active_bricks] = (
            ((x & 0xFF)) |
            ((y & 0xFF) << 8) |
            ((z & 0xFF) << 16)
          );
          state->active_bricks++;
        }
      }
    }
    igText("active bricks: %u", state->active_bricks);


    rawkit_gpu_copy_buffer(
      rawkit_default_gpu(),
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      state->positions->staging_buffer,
      state->positions->buffer,
      state->positions->buffer->size
    );
  }

  auto spammer = rawkit_shader(
    rawkit_file("spam.vert"),
    rawkit_file("spam.frag")
  );

  auto inst = rawkit_shader_instance_begin(spammer);
  if (inst) {

    rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
    rawkit_shader_instance_param_ssbo(inst, "Positions", state->positions);

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
    if (state->active_bricks > 2) {
      vkCmdDraw(
        inst->command_buffer,
        36,
        state->active_bricks,
        0, // firstIndex
        0
      );
    }

    rawkit_shader_instance_end(inst);
  }
}