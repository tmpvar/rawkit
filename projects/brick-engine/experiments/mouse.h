#pragma once
#include <rawkit/rawkit.h>
#include <glm/glm.hpp>

struct Mouse {
  glm::vec2 pos;
  glm::vec2 last_pos;
  glm::vec2 down_pos;
  bool down;
  bool was_down;

  void tick() {
    this->last_pos = this->pos;
    this->was_down = this->down;

    double mx, my;
    GLFWwindow *window = rawkit_glfw_window();
    ImGuiIO *io = igGetIO();
    glfwGetCursorPos(window, &mx, &my);
    this->pos = glm::vec2(
      mx,
      my
    );

    if (io && !io->WantCaptureMouse) {
      if (igIsMouseDown(ImGuiMouseButton_Left)) {
        if (!this->down) {
          this->down_pos = this->pos;
        }
        this->down = true;
      } else {
        this->down = false;
      }
    }
    this->debug();
  }

  void debug() {
    igText("mouse ::\n  pos(%f, %f)\n  down_pos(%f, %f)\n  down=%i\n  was_down=%i",
    this->pos.x, this->pos.y,
    this->down_pos.x, this->down_pos.y,
    this->down,
    this->was_down
    );
  }
};