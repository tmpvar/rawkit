#include <rawkit/rawkit.h>
#include "../fg/framegraph.h"

void graph_texture(FrameGraph *fg) {
  auto tex = fg->texture(
    "mem-texture",
    VK_FORMAT_R8G8B8A8_SRGB,
    uvec3(
      rawkit_window_width(),
      rawkit_window_height(),
      1
    )
  );

  fg->shader("fill-texture", {"texture.comp"})
  ->dispatch(
    tex->dims(),
    {
      { "tex", tex },
    }
  );

  tex->debug_imgui();

  // moss texture
  {
    auto moss = fg->texture("moss.png");
    moss->debug_imgui();
  }
}