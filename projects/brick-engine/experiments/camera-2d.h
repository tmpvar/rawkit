#pragma once

#include "context-2d.h"

#include <glm/glm.hpp>

struct Camera2D {
  Context2D ctx;

  bool is_setup = false;
  bool is_down = false;
  glm::vec2 down_pos = vec2(0.0);

  glm::vec2 translation = vec2(0.0);
  float scale = 1.0f;

  glm::vec2 scale_limits = vec2(0.2, 100);

  // if using this with hotstate, we need to setup the initial state
  void setup(float initial_scale = 1.0f, glm::vec2 scale_limits = vec2(0.2, 100)) {
    if (this->is_setup) {
      return;
    }
    this->is_setup = true;
    this->scale = initial_scale;
    this->scale_limits = scale_limits;
    this->ctx = Context2D();
  }

  void tick(bool down, glm::vec2 pos, float wheel) {
    this->zoomToScreenPoint(pos, wheel);

    if (!this->is_down) {
      if (down) {
        this->is_down = true;
        this->down_pos = pos;
      }
      return;
    }

    if (this->is_down && !down) {
      this->is_down = false;
      return;
    }

    this->translate((pos - this->down_pos) / this->scale);
    this->down_pos = pos;
  }

  void zoomToScreenPoint (vec2 pos, float amount) {
    glm::vec2 o = pos / this->scale;
    this->zoom(amount);
    glm::vec2 n = pos / this->scale;
    this->translate(n - o);
  }

  void translate(glm::vec2 delta) {
    this->translation += delta;
  }

  void zoom(float amount) {
    this->scale = glm::clamp(
      this->scale + amount,
      this->scale_limits.x,
      this->scale_limits.y
    );
  }

  void begin() {
    this->ctx.save();
      this->ctx.scale(vec2(this->scale));
      this->ctx.translate(this->translation);
  }

  void end() {
    this->ctx.restore();
  }

};