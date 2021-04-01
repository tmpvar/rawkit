#define CPU_HOST

#include <rawkit/rawkit.h>
#include "nanovdb/util/Primitives.h"
#include "nanovdb/util/IO.h"

#include "shared.h"
#include "vdb.h"
#include "prof.h"

#include "../camera.hpp"
#define MAX_DEPTH 10000
struct State {
  rawkit_gpu_ssbo_t *data;
  Camera *camera;
  Scene scene;
  vec2 last_mouse_pos;
  double last_time;
};

void setup() {
  State *state = rawkit_hot_state("state", State);

  if (!state->camera) {
    state->camera = new Camera;
    state->camera->type = Camera::CameraType::firstperson;
    state->camera->flipY = true;
    state->camera->setTranslation(vec3(0.0, 0.0, -100));
    state->camera->setRotation(vec3(0.0, 0.0, 0.0));
  }

  try {
    Prof build("build");
    auto handle = nanovdb::createLevelSetSphere<float>(512.0f);

    state->data = rawkit_gpu_ssbo("handle::ssbo", handle.size());
    u8 *data = handle.data();

    rawkit_gpu_ssbo_update(
      state->data,
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      data,
      handle.size()
    );

    state->scene.data_size = handle.size();

    printf("update ssbo %llu bytes version:%u first byte: %u\n", handle.size(), state->data->resource_version, handle.data()[0]);
    {
      FILE *f = fopen("data.nvdb", "w+b");
      fwrite(handle.data(), 1, handle.size(), f);
      fclose(f);
    }

    build.finish();
    // grid->tree().root().tile(0).childID;
    // auto tree = grid->tree().root();//.root()->child(grid->tree().root()->tile(0)).child(0)->child(0)->getValue(nanovdb::Coord(0, 0, 0));
    // grid->blindData(0)
  }
  catch (const std::exception& e) {
      std::cerr << "An exception occurred: \"" << e.what() << "\"" << std::endl;
  }
}

void updateCamera(State *state) {
  double now = rawkit_now();
  double dt = now - state->last_time;
  state->last_time = now;

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
    } else {
      igText("no window...");
    }
  }

  // double move = dt * 100.0;

  // w
  state->camera->keys.up = igIsKeyDown(87);

  // a
  state->camera->keys.left = igIsKeyDown(65);

  // s
  state->camera->keys.down = igIsKeyDown(83);

  // d
  state->camera->keys.right = igIsKeyDown(68);

  float speed = 100;
  // left shift
  if (igIsKeyDown(340)) {
    state->camera->setMovementSpeed(speed * 10.0f);
  } else {
    state->camera->setMovementSpeed(speed);
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

// render the texture to a full screen surface
void renderFullscreenTexture(State *state, rawkit_texture_t *tex_color) {
  rawkit_shader_t *shader = rawkit_shader(
    rawkit_file("shader/fullscreen.vert"),
    rawkit_file("shader/fullscreen.frag")
  );

  rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);
  if (inst) {
    rawkit_shader_instance_param_texture(inst, "tex_color", tex_color, nullptr);

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
    vkCmdDraw(inst->command_buffer, 3, 1, 0, 0);
    rawkit_shader_instance_end(inst);
  }
}

void loop() {
  auto state = rawkit_hot_state("state", State);

  state->scene.screen_dims = vec4(
    (float)rawkit_window_width(),
    (float)rawkit_window_height(),
    0,
    0
  );

  updateCamera(state);


  rawkit_texture_t *tex_color = rawkit_texture_mem(
    "renderer::raytrace::tex-color",
    state->scene.screen_dims.x,
    state->scene.screen_dims.y,
    1,
    VK_FORMAT_R32G32B32A32_SFLOAT
  );


  rawkit_shader_t *raytrace_shader = rawkit_shader(
    rawkit_file("shader/raytrace.comp")
  );

  rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
    rawkit_default_gpu(),
    raytrace_shader,
    NULL,
    0
  );

  if (inst) {
    rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
    rawkit_shader_instance_param_ssbo(inst, "Data", state->data);
    rawkit_shader_instance_param_texture(inst, "tex_color", tex_color, nullptr);
    rawkit_shader_instance_dispatch_compute(
      inst,
      state->scene.screen_dims.x,
      state->scene.screen_dims.y,
      1
    );

    rawkit_shader_instance_end(inst);
  }

  renderFullscreenTexture(state, tex_color);
}