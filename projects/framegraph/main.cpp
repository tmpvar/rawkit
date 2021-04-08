
#include <rawkit/rawkit.h>
#include "fg/framegraph.h"
#include <glm/glm.hpp>

#include "ctx/context-2d.h"
#include "ctx/camera-2d.h"
#include "ctx/mouse.h"

using namespace glm;

struct State {
  FrameGraph *fg;

};

void setup() {
  State *state = rawkit_hot_state("state", State);
  if (!state->fg) {
    state->fg = new FrameGraph();
  }

}
void loop() {
  State *state = rawkit_hot_state("state", State);
  FrameGraph *fg = state->fg;

  fg->ring_buffer->_state->write_offset = 0;
  fg->begin();
  state->fg->queue = rawkit_vulkan_queue();
  state->fg->command_pool = rawkit_vulkan_command_pool();


  // rawkit_default_gpu(),
  // default_memory_flags | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
  // default_buffer_usage_flags

  auto input_buf = fg->buffer<u32>("inputs", 64);
  input_buf->write({
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
  });

  auto output_buf = fg->buffer<u32>("outputs", 4);
  output_buf->fill(0);

  fg->shader("sum", {"sum.comp"})
    ->dispatch(
      uvec3(input_buf->length, 1, 1),
      {
        { "input_buf", input_buf },
        { "output_buf", output_buf },
      }
    );

  output_buf->read(0, 4, [](u32 *data, u32 size) {
    igText("0: %u", data[0]);
    igText("1: %u", data[1]);
    igText("2: %u", data[2]);
    igText("3: %u", data[3]);
  });

  fg->end();

  vkWaitForFences(
    rawkit_default_gpu()->device,
    1,
    &fg->fence,
    VK_TRUE,
    UINT64_MAX
  );

  {
    auto data = (u32 *)fg->ring_buffer->data(0);
    printf("readback late\n  ");
    for (u32 i=0; i<128; i++) {
      if (i > 0 && i%16 == 0) {
        printf("\n  ");
      }
      printf("%u ", data[i]);
    }
    printf("\n------------------------------------------\n");
  }

  // output .dot format
  {
    static bool first = true;
    if (first) {
      first = false;
      const char *fill_color = "#BBBBBBFF";
      const char *border_color = "#AAAAAAFF";
      const char *font_color = "#000000FF";
      const char *edge_color = "#666666FF";

      printf(
        "digraph framegraph {\n"
        "  concentrate=true\n"
        "  graph [truecolor=true bgcolor=\"#00000000\"]\n"
        "  node [style=filled fillcolor=\"%s\" color=\"%s\" fontcolor=\"%s\" fontname=\"Roboto Light\"]\n"
        "  edge [color=\"%s\"]\n",
        fill_color,
        border_color,
        font_color,
        edge_color
      );

      for (auto node : fg->nodes) {
        u32 idx = node->node();
        string params = string("label=\"") + node->name + "\"";

        switch (node->node_type) {
          case NodeType::SHADER_INVOCATION:
              params += " shape=square style=\"bold,filled\"";
            break;
          case NodeType::SHADER:
              params += " shape=square style=\"filled\"";
            break;
          default:
            params += " shape=box style=\"rounded,filled\"";
        }

        printf("  node_%u [%s];\n", idx, params.c_str());
      }

      for (auto node : fg->nodes) {
        u32 idx = node->node();

        auto inputs = fg->input_edges.find(idx);
        if (inputs != fg->input_edges.end()) {
          for (auto input : inputs->second) {
            printf("  node_%u -> node_%u;\n", input, idx);
          }
        }
      }
      printf("}\n");
    }
  }

  fg->render_force_directed_imgui();
}