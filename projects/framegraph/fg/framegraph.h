#pragma once

#include <rawkit/rawkit.h>

#include "buffer.h"
#include "shader.h"
#include "texture.h"
#include "ring-buffer.h"

#include <vector>
using namespace std;
struct FrameGraph {
  vector<Shader *>shaders;
  RingBuffer *ring_buffer;

  FrameGraph() {
    this->ring_buffer = new RingBuffer("FrameGraph::ring_buffer", 64 * 1024 * 1024);
  }

  ~FrameGraph() {
    delete this->ring_buffer;
  }

  Shader &shader(const char *name, vector<const char *> filenames) {
    Shader *s = new Shader(name, filenames);
    this->shaders.push_back(s);
    return *s;
  }
};