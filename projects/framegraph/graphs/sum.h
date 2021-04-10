#include <rawkit/rawkit.h>
#include "../fg/framegraph.h"

void graph_sum(FrameGraph *fg) {
  auto input_buf = fg->buffer<u32>("inputs", 64);
  input_buf->write({
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
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
    igBegin("sum results:", 0, 0);
    igText("  0: %u", data[0]);
    igText("  1: %u", data[1]);
    igText("  2: %u", data[2]);
    igText("  3: %u", data[3]);
    igEnd();
  });
}