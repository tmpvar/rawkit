#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <functional>

#include <glm/glm.hpp>
using namespace glm;

#include "prof.h"
#include "state.h"

#include "renderer/vg.h"
#include "renderer/lines.h"

void setup() {
  State *state = rawkit_hot_state("state", State);
  state->camera2d.setup(1.0f, vec2(.01, 10000.0));

  if (!state->camera) {
    state->camera = new Camera;
    state->camera->type = Camera::CameraType::firstperson;
    state->camera->flipY = true;
    state->camera->setTranslation(vec3(0.0));
    state->camera->setRotation(vec3(0.0, 130.0, 0.0));
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

i32 fillInner(State *state, const float radius, const vec3 center, sample_fn_t sample, u32 depth) {
  const u32 max_depth = 12;
  if (depth > max_depth) {
    return -1;
  }
  float child_radius = radius * 0.5f;
  // TODO: if the children will be leaves then fillLeaf
  if (radius < 16.0f) {
    return -2;
  }

  float dist = sample_scene(center);
  if (abs(dist) >= glm::length(vec3(radius))) {
    return -3;
  }

  sb_push(state->node_positions, vec4(center, radius));

  InnerNode node = {};

  // test the octants and recurse as necessary
  bool valid = false;
  for (u8 i = 0; i<8; i++) {
    float x = (i&1<<0) == 0 ? -child_radius : child_radius;
    float y = (i&1<<1) == 0 ? -child_radius : child_radius;
    float z = (i&1<<2) == 0 ? -child_radius : child_radius;
    vec3 p = center + vec3(x, y, z);
    i32 r = fillInner(state, child_radius, p, sample, depth+1);

    if (r > -1) {
      valid = true;
    }

    node.children[i] = r;
  }

  // if (!valid) {
  //   return -1;
  // }

  i32 c = sb_count(state->nodes);
  sb_push(state->nodes, node);

  return c;
}

void loop() {
  Prof loop_prof("loop");
  State *state = rawkit_hot_state("state", State);
  state->mouse.tick();
  double now = rawkit_now();
  double dt = now - state->last_time;
  state->last_time = now;

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
      state->camera->setMovementSpeed(1000.0);
    } else {
      state->camera->setMovementSpeed(100.0);
    }

    igShowDemoWindow(0);
    // // space
    // if (igIsKeyDown(341)) {
    //   camera->pos.y -= move;
    // }

    // // control
    // if (igIsKeyDown(32)) {
    //   camera->pos.y += move;
    // }

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
  }

  // recalculate the tree on every frame
  state->scene.tree_radius = 0.0f;
  {
    Prof rebuild_prof("rebuild");
    sb_reset(state->leaves);
    sb_reset(state->nodes);
    sb_reset(state->node_positions);
    sb_reset(state->ops);

    // TODO: compute the bounds
    sb_push(state->ops, vec4(0, 0, 0, 128));

    float now = rawkit_now();

    sb_push(state->ops, vec4(
      sinf(now) * 1200.0f,
      cosf(now) * 1200.0f,
      0,
      90
    ));

    sb_push(state->ops, vec4(
      200 + sinf(now) * 100.0f,
      200 + cosf(now * 0.5f) * 100.0f,
      0,
      90
    ));

    sb_push(state->ops, vec4(
      4600 + sinf(now * 0.25) * 1600.0f,
      200 + cosf(now * 0.5f) * 100.0f,
      0,
      256
    ));

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

    const vec3 sdf_center(0.0);
    state->scene.tree_radius = next_power_of_two(sdf_radius);
    fillInner(state, state->scene.tree_radius, sdf_center, sample_scene, 0);
  }

  // transfer node tree to gpu
  {
    u64 size = sb_count(state->nodes) * sizeof(InnerNode);
    state->nodes_ssbo = rawkit_gpu_ssbo("node::tree", size);
    rawkit_gpu_ssbo_update(
      state->nodes_ssbo,
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      state->nodes,
      size
    );
  }

  // transfer node tree to gpu
  {
    u64 size = sb_count(state->node_positions) * sizeof(vec4);
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
      default:
        igText("invalid renderer selected");
    }
  }
}