
#include <rawkit/core.h>
#include <rawkit/hot.h>
#include <rawkit/gpu.h>
#include <vulkan/vulkan.h>
#include <rawkit/window.h>

#include <GLFW/glfw3.h>


class WindowState {
  public:
    GLFWwindow *window;

    WindowState() {

    }
};

void rawkit_window_init(int width, int height, const char *title) {
  // Note: we currently only support a single window!
  rawkit_window_t *res = rawkit_hot_resource("rawkit::window", rawkit_window_t);
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