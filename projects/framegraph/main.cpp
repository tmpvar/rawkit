
#include <rawkit/rawkit.h>
#include "fg/framegraph.h"
#include <glm/glm.hpp>

#include "ctx/context-2d.h"
#include "ctx/camera-2d.h"
#include "ctx/mouse.h"

#include "graphs/sum.h"
#include "graphs/texture.h"

using namespace glm;

struct State {
  FrameGraph *fg;

};

void setup() {
  State *state = rawkit_hot_state("state", State);

  // TODO: tuck this footgun away (FrameGraph::create(name))
  if (!state->fg) {
    state->fg = new FrameGraph();
  } else {
    // We MUST reset the framegraph on subsequent reloads to avoid calling into
    // old memory (e.g. deleted callbacks)
    state->fg->reset();
  }

  state->fg->queue = rawkit_vulkan_queue();
  state->fg->command_pool = rawkit_vulkan_command_pool();
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  FrameGraph *fg = state->fg;
  fg->begin();

  graph_sum(fg);
  graph_texture(fg);


  fg->render_force_directed_imgui();
  fg->end();
}