
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
  fg->render_force_directed_imgui();
}