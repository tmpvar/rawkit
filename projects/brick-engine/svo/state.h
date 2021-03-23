#pragma once

#define CPU_HOST
#include "svo.h"
#include "../experiments/context-2d.h"
#include "../experiments/camera-2d.h"
#include "../experiments/mouse.h"


struct State {
  Camera2D camera;
  Mouse mouse;
  InnerNode *nodes;
  LeafNode *leaves;
  vec4 *node_positions;
  vec4 *ops;
};
