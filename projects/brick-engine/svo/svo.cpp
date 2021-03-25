#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <functional>

#include <glm/glm.hpp>
using namespace glm;

#include "prof.h"
#include "state.h"

#include "renderer/vg.h"
#include "renderer/lines.h"
#include "renderer/raytrace.h"

bool rebuild = false;
void setup() {
  rebuild = true;
  State *state = rawkit_hot_state("state", State);
  state->camera2d.setup(1.0f, vec2(.01, 10000.0));

  if (!state->camera) {
    state->camera = new Camera;
    state->camera->type = Camera::CameraType::firstperson;
    state->camera->flipY = true;
    state->camera->setTranslation(vec3(0.0, 0.0, -4096));
    state->camera->setRotation(vec3(0.0, 0.0, 0.0));
  }
}

typedef std::function<float(vec3 pos)> sample_fn_t;

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

float sample_scene(vec3 pos) {
  State *state = rawkit_hot_state("state", State);
  float d = FLT_MAX;
  u32 c = sb_count(state->ops);
  for (u32 i=0; i<c; i++) {
    vec4 op = state->ops[i];
    d = glm::min(
      d,
      distance(pos, vec3(op)) - op.w
    );
  }

  return d;
}


void fillLeaf(State *state, const InnerNode *parent, const vec3 parent_pos, sample_fn_t sample) {



}

i32 fillInner(State *state, const float radius, const vec3 center, sample_fn_t sample_fn, u32 depth) {
  if (depth > 23) {
    return 0;
  }

  float child_radius = radius * 0.5f;
  // TODO: if the children will be leaves then fillLeaf
  if (radius < 4.0f) {
    return 0;
  }

  float dist = sample_fn(center);
  if (abs(dist) >= glm::length(vec3(radius))) {
    return -1;
  }

  sb_push(state->node_positions, vec4(center, radius));

  i32 c = sb_count(state->nodes);
  InnerNode n = {};
  sb_push(state->nodes, n);
  // test the octants and recurse as necessary
  bool valid = false;
  for (u8 i = 0; i<8; i++) {

    // extract morton order into 3d coordinates
    vec3 p = center + vec3(
      (i&1<<0) == 0 ? -child_radius : child_radius,
      (i&1<<1) == 0 ? -child_radius : child_radius,
      (i&1<<2) == 0 ? -child_radius : child_radius
    );

    i32 r = fillInner(state, child_radius, p, sample_fn, depth+1);
    if (r > 0) {
      valid = true;
    }

    // state->nodes will likely get realloc'd so we need to use the index instead
    // to avoid reading freed memory!
    state->nodes[c].children[i] = r;
  }

  // if (!valid) {
  //   return -1;
  // }


  return c;
}

void loop() {
  Prof loop_prof("loop");
  State *state = rawkit_hot_state("state", State);
  state->mouse.tick();
  double now = rawkit_now();
  double dt = now - state->last_time;
  state->last_time = now;
  state->renderer = Renderer::RAYTRACE;
  // mouse look
  {
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

  // update 3d camera
  {
    state->scene.screen_dims = vec4(
      (float)rawkit_window_width(),
      (float)rawkit_window_height(),
      0,
      0
    );

    // double move = dt * 100.0;

    // w
    state->camera->keys.up = igIsKeyDown(87);

    // a
    state->camera->keys.left = igIsKeyDown(65);

    // s
    state->camera->keys.down = igIsKeyDown(83);

    // d
    state->camera->keys.right = igIsKeyDown(68);

    // left shift
    if (igIsKeyDown(340)) {
      state->camera->setMovementSpeed(10000.0);
    } else {
      state->camera->setMovementSpeed(1000.0);
    }

    // // space
    // if (igIsKeyDown(341)) {
    //   camera->pos.y -= move;
    // }

    // // control
    // if (igIsKeyDown(32)) {
    //   camera->pos.y += move;
    // }


    float fovDegrees = 90.0f;
    state->camera->setPerspective(
      fovDegrees,
      state->scene.screen_dims.x / state->scene.screen_dims.y,
      0.1f,
      (float)MAX_DEPTH
    );

    state->scene.pixel_size = glm::tan(glm::radians(fovDegrees) / 2.0f);

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

    state->camera->update(dt);

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

    state->scene.proj = state->camera->matrices.perspective;
    state->scene.view = state->camera->matrices.view;
    state->scene.worldToScreen = clip * state->camera->matrices.perspective * state->camera->matrices.view;
    state->scene.eye = vec4(state->camera->position, 1.0);

    state->scene.eye = state->camera->viewPos;
    igText("eye(%f, %f, %f)", state->scene.eye.x, state->scene.eye.y, state->scene.eye.z);
  }

  // recalculate the tree on every frame
  if (rebuild || !sb_count(state->nodes)) {
    rebuild = false;
    state->scene.tree_radius = 0.0f;
    Prof rebuild_prof("rebuild");
    sb_reset(state->leaves);
    sb_reset(state->nodes);
    sb_reset(state->node_positions);
    sb_reset(state->ops);

    // sb_push(state->ops, vec4(0, 0, 0, 512));

    float now = rawkit_now();

    // sb_push(state->ops, vec4(
    //   sinf(now) * 1200.0f,
    //   cosf(now) * 1200.0f,
    //   0,
    //   90
    // ));

    // sb_push(state->ops, vec4(
    //   200 + sinf(now) * 100.0f,
    //   200 + cosf(now * 0.5f) * 100.0f,
    //   0,
    //   90
    // ));

    // sb_push(state->ops, vec4(
    //   4600 + sinf(now * 0.25) * 1600.0f,
    //   200 + cosf(now * 0.5f) * 100.0f,
    //   0,
    //   256
    // ));

    for (float i=0; i<100; i++) {
      sb_push(state->ops, vec4(
        500 + cosf(i * 0.5f) * i*8.0f,
        200 + sinf(i * 0.5f) * i*8.0f,
        i*128.0,
        512 - i *2.0f
      ));
    }

    float sdf_radius = 0.0f;
    {
      u32 c = sb_count(state->ops);
      for (u32 i=0; i<c; i++) {
        vec4 op = state->ops[i];
        sdf_radius = glm::max(
          sdf_radius,
          length(vec3(op)) + op.w
        );
      }
    }

    state->scene.tree_center = vec3(0.0);
    state->scene.tree_radius = next_power_of_two(sdf_radius);
    fillInner(state, state->scene.tree_radius, state->scene.tree_center, sample_scene, 0);
  }


  // transfer node tree to gpu
  {
    u64 size = next_power_of_two(sb_count(state->nodes) * sizeof(InnerNode));
    state->nodes_ssbo = rawkit_gpu_ssbo("node::tree", size);
    rawkit_gpu_ssbo_update(
      state->nodes_ssbo,
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      state->nodes,
      size
    );
  }

  // transfer node positions to gpu
  {
    u64 size = next_power_of_two(sb_count(state->node_positions) * sizeof(vec4));
    state->node_positions_ssbo = rawkit_gpu_ssbo("node::positions", size);
    rawkit_gpu_ssbo_update(
      state->node_positions_ssbo,
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      state->node_positions,
      size
    );
  }

  // visualize
  {
    igText("tree_radius: %f dense voxels: %0.1fB", state->scene.tree_radius, glm::pow(state->scene.tree_radius, 3.0) / glm::pow(1000, 3));
    igText("nodes inner(%llu) leaf(%llu)", sb_count(state->nodes), sb_count(state->leaves));

    switch (state->renderer) {
      case Renderer::VG: renderer_vg(state, state->scene.tree_radius); break;
      case Renderer::LINES: renderer_lines(state, state->scene.tree_radius); break;
      case Renderer::RAYTRACE: renderer_raytrace(state, state->scene.tree_radius); break;
      default:
        igText("invalid renderer selected");
    }
  }
}