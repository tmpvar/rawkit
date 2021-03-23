#include "../state.h"

void renderer_vg(State *state, float tree_radius) {
  Context2D ctx;
  state->camera2d.ctx = ctx;

  state->camera2d.tick(
    igIsMouseDown(ImGuiMouseButton_Middle),
    state->mouse.pos,
    state->mouse.wheel * 0.1f
  );

  state->camera2d.begin();

  ctx.translate(vec2(800));

  u32 c = sb_count(state->node_positions);
  // render the grid
  {
    ctx.strokeColor(rgb(0xFFFFFFFF));
    for (u32 i=0; i<c; i++) {
        vec4 pos = state->node_positions[i];
        float r = pos.w;
        if (pos.w < 8) {
            continue;
        }
        ctx.beginPath();
            ctx.rect(vec2(pos) - r, vec2(r * 2.0f));
        ctx.closePath();
        ctx.strokeColor(hsl(pos.w / tree_radius, 0.9, 0.6));
        ctx.stroke();
    }
  }

  // render the ops (circles for now)
  {
    ctx.strokeWidth(1.0f);
    ctx.strokeColor(rgb(0xFFFFFFFF));
    u32 c = sb_count(state->ops);
    ctx.beginPath();
    for (u32 i=0; i<c; i++) {
      vec4 op = state->ops[i];
      ctx.moveTo(vec2(op.x + op.w, op.y));
      ctx.arc(vec2(op), op.w);
    }
    ctx.stroke();
  }

  state->camera2d.end();
}