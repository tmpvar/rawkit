
#include <rawkit/core.h>
#include <rawkit/hot.h>
#include <rawkit/gpu.h>
#include <vulkan/vulkan.h>
#include <rawkit/window.h>

#include <GLFW/glfw3.h>


class WindowState {
  public:
    GLFWwindow *window;
    uint32_t frame_index;
    uint32_t frame_count;
    WindowState() {

    }
};

static const char *resource_name = "rawkit::window";
void rawkit_window_init(int width, int height, const char *title) {
  // Note: we currently only support a single window!
  rawkit_window_t *res = rawkit_hot_resource(resource_name, rawkit_window_t);
  if (!res->_state) {
    res->_state = (void *)new WindowState;
  }

  if (!res->_state) {
    printf("ERROR: rawkit_window_init: invalid internal state\n");
    return;
  }

  WindowState *state = (WindowState *)res->_state;

  glfwSetWindowTitle(state->window, title);
  glfwSetWindowSize(state->window, width, height);
}

uint32_t rawkit_window_frame_index() {
  rawkit_window_t *window = rawkit_hot_resource(resource_name, rawkit_window_t);
  if (!window || !window->_state) {
    return 0;
  }

  WindowState *state = (WindowState *)window->_state;
  return state->frame_index;
}

uint32_t rawkit_window_frame_count() {
  rawkit_window_t *window = rawkit_hot_resource(resource_name, rawkit_window_t);
  if (!window || !window->_state) {
    return 0;
  }

  WindowState *state = (WindowState *)window->_state;
  return state->frame_count;
}

GLFWwindow *rawkit_glfw_window() {
  rawkit_window_t *window = rawkit_hot_resource(resource_name, rawkit_window_t);
  if (!window || !window->_state) {
    return NULL;
  }

  WindowState *state = (WindowState *)window->_state;
  return state->window;
}

// INTERNAL

void rawkit_window_internal_set_frame_index(uint32_t val) {
  rawkit_window_t *window = rawkit_hot_resource(resource_name, rawkit_window_t);
  if (!window) {
    return;
  }

  if (!window->_state) {
    window->_state = (void*)new WindowState;
  }

  WindowState *state = (WindowState *)window->_state;
  state->frame_index = val;
}

void rawkit_window_internal_set_frame_count(uint32_t val) {
  rawkit_window_t *window = rawkit_hot_resource(resource_name, rawkit_window_t);
  if (!window) {
    return;
  }

  if (!window->_state) {
    window->_state = (void*)new WindowState;
  }

  WindowState *state = (WindowState *)window->_state;
  state->frame_count = val;
}


void rawkit_window_internal_set_glfw_window(GLFWwindow *glfw_window) {
  rawkit_window_t *window = rawkit_hot_resource(resource_name, rawkit_window_t);
  if (!window) {
    return;
  }

  if (!window->_state) {
    window->_state = (void*)new WindowState;
  }

  WindowState *state = (WindowState *)window->_state;
  state->window = glfw_window;
}