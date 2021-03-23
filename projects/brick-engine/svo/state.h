#pragma once

#define CPU_HOST
#include "svo.h"
#include "../experiments/context-2d.h"
#include "../experiments/camera-2d.h"
#include "../experiments/mouse.h"

#include "../camera.hpp"

enum Renderer {
  LINES = 0,
  VG,
};

struct State {
  Camera *camera;
  Camera2D camera2d;

  vec2 last_mouse_pos;

  Mouse mouse;
  LeafNode *leaves;
  vec4 *ops;

  InnerNode *nodes;
  rawkit_gpu_ssbo_t *nodes_ssbo;

  vec4 *node_positions;
  rawkit_gpu_ssbo_t *node_positions_ssbo;

  Renderer renderer;

  Scene scene;

  double last_time;
};
