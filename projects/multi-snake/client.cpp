#include <rawkit/rawkit.h>
#include <glm/glm.hpp>

#include <stb_sb.h>
using namespace glm;

#include "context-2d.h"
#include "tcp.h"

#include "shared.h"

struct Snake {
  vec2 *points;
  vec2 head;
  Color color;
  u32 length;
};

enum CellType {
  EMPTY = 0,
  SNAKE = 1,
  APPLE = 2,
};

struct Apple {
  vec2 pos;
  bool eaten;
};

struct Cell {
  CellType type;
  u32 idx;
};

const u8 UP    = 0;
const u8 DOWN  = 1;
const u8 LEFT  = 2;
const u8 RIGHT = 3;

struct State {
  Snake *snakes;
  u8 input;
  u32 ticker;

  Apple *apples;
  TCPClient *client;
};

void setup() {
  State *state = rawkit_hot_state("state", State);

  if (!state->client) {
    printf("here\n");
    state->client = new TCPClient("127.0.0.1", 3030);
    printf("and here\n");
  }

  if (!sb_count(state->snakes)) {
    Snake a = {};
    a.length = 10;
    a.color = rgb(0xFF, 0xFF, 0xFF);
    sb_push(state->snakes, a);
  }

  if (!sb_count(state->apples)) {
    for (u32 i=0; i<10; i++) {
      Apple apple = {
        .pos = vec2(
          floor(rawkit_randf() * (float)grid_dims.x),
          floor(rawkit_randf() * (float)grid_dims.y)
        ),
        .eaten = false
      };

      sb_push(state->apples, apple);
    }
  }
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  state->ticker++;
  igShowDemoWindow(0);

  // service the tcp connection
  {
    i32 sentinel = 10000;
    ps_val_t *val = nullptr;
    while (sentinel--) {
      val = state->client->read();
      if (!val) {
        break;
      }

      ps_destroy(val);
    };

    const char *str = "tick";
    state->client->write_copy((const u8 *)str, strlen(str));
  }



  rawkit_cpu_buffer_t *grid_buf = rawkit_cpu_buffer("grid_buf", grid_dims.x * grid_dims.y * sizeof(Cell));
  rawkit_texture_t *grid_tex = rawkit_texture_mem(
    "grid_tex",
    grid_dims.x,
    grid_dims.y,
    1,
    VK_FORMAT_R32G32B32A32_SFLOAT
  );

  rawkit_cpu_buffer_t *grid_tex_buf = rawkit_cpu_buffer("grid_tex_buf", grid_tex->options.size);

  const rawkit_texture_sampler_t *nearest_sampler = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_NEAREST,
    VK_FILTER_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    0.0f,
    false,
    0.0f,
    false,
    VK_COMPARE_OP_NEVER,
    0,
    1,
    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    false
  );


  // inputs
  {
    if (igIsKeyDown(0x109) && state->input != DOWN) {
      state->input = UP;
    }

    if (igIsKeyDown(0x108) && state->input != UP) {
      state->input = DOWN;
    }

    if (igIsKeyDown(0x107) && state->input != RIGHT) {
      state->input = LEFT;
    }
    if (igIsKeyDown(0x106) && state->input != LEFT) {
      state->input = RIGHT;
    }
  }

  // apply input to points
  {
    if (state->ticker % 5 == 0) {
      switch (state->input) {
        case UP:
          state->snakes[0].head.y = glm::mod(
            state->snakes[0].head.y + 1,
            (float)grid_dims.y
          );
          break;
        case DOWN:
          state->snakes[0].head.y = glm::mod(
            state->snakes[0].head.y - 1.0f,
            (float)grid_dims.y
          );
          break;
        case LEFT:
          state->snakes[0].head.x = glm::mod(
            state->snakes[0].head.x - 1.0f,
            (float)grid_dims.x
          );
          break;
        case RIGHT:
          state->snakes[0].head.x = glm::mod(
            state->snakes[0].head.x + 1.0f,
            (float)grid_dims.x
          );
          break;
      }

      u32 c = sb_count(state->snakes[0].points);
      if (c < state->snakes[0].length) {
        sb_push(state->snakes[0].points, state->snakes[0].head);
      } else {
        for (u32 i=1; i < c; i++) {
          state->snakes[0].points[i-1] = vec2(state->snakes[0].points[i]);
        }
        state->snakes[0].points[c-1] = vec2(state->snakes[0].head);
      }
    }
  }


  // reset grid
  {
    memset(grid_buf->data, 0, grid_buf->size);
    memset(grid_tex_buf->data, 0, grid_tex_buf->size);
  }

  // paint apples into world grid
  {
    u32 c = sb_count(state->apples);
    for (u32 i=0; i<c; i++) {
      Apple *apple = &state->apples[i];
      if (apple->eaten) {
        apple->eaten = false;
        apple->pos = vec2(
          floor(rawkit_randf() * (float)grid_dims.x),
          floor(rawkit_randf() * (float)grid_dims.y)
        );
      }

      // collision grid
      {
        u32 idx = apple->pos.x + apple->pos.y * grid_dims.x;

        Cell *cell = &((Cell *)grid_buf->data)[idx];
        cell->type = CellType::APPLE;
        cell->idx = i;
      }

      // visual grid
      {
        u32 idx = u32(apple->pos.x * 4 + apple->pos.y * grid_dims.x * 4);
        ((float *)grid_tex_buf->data)[idx + 0] = 1.0f;
        ((float *)grid_tex_buf->data)[idx + 1] = 0.0f;
        ((float *)grid_tex_buf->data)[idx + 2] = 0.0f;
        ((float *)grid_tex_buf->data)[idx + 3] = 1.0f;
      }
    }
  }

  Context2D ctx;
  // paint snakes into world grid
  {

    u32 snake_count = sb_count(state->snakes);
    for (u32 sidx=0; sidx<snake_count; sidx++) {
      Color color = state->snakes[sidx].color;

      {
        vec2 head = glm::mod(state->snakes[sidx].head, vec2(grid_dims));
        u32 p = u32(head.x + head.y * grid_dims.x);
        Cell *cell = &((Cell *)grid_buf->data)[p];
        switch (cell->type) {
          case CellType::APPLE: {
            Apple *apple = &state->apples[cell->idx];
            state->snakes[sidx].length += 10.0f;
            state->apples[cell->idx].eaten = true;
            break;
          }
          default:
            break;
        }
      }

      u32 point_count = sb_count(state->snakes[sidx].points);
      for (u32 pidx=0; pidx<point_count; pidx++) {
        vec2 point = glm::mod(state->snakes[sidx].points[pidx], vec2(grid_dims));
        u32 p = u32(
          point.x + point.y * grid_dims.x
        );
        if (p > grid_buf->size) {
          continue;
        }

        Cell *cell = &((Cell *)grid_buf->data)[p];
        cell->type = CellType::SNAKE;
        cell->idx = sidx;

        u32 i = u32(point.x * 4 + point.y * grid_dims.x * 4);
        ((float *)grid_tex_buf->data)[i + 0] = color.value.r;
        ((float *)grid_tex_buf->data)[i + 1] = color.value.g;
        ((float *)grid_tex_buf->data)[i + 2] = color.value.b;
        ((float *)grid_tex_buf->data)[i + 3] = color.value.a;
      }
    }
    rawkit_texture_update_buffer(grid_tex, grid_tex_buf);
  }

  ctx.scale(vec2(1.0f, -1.0f));
  ctx.translate(vec2(0.0, -(float)rawkit_window_height()));
  ctx.drawTexture(vec2(20.0), vec2(grid_dims) * 6.0f, grid_tex, nearest_sampler);

    // ctx.strokeColor(rgb(0xFF, 0xFF, 0xFF));

    // ctx.translate(vec2(500, 500));
    // ctx.beginPath();
    //   ctx.moveTo(vec2(0.0));
    //   ctx.lineTo(state->snakes[0].dir * 100.0f);

    //   // ctx.lineTo(100.0f + state->snakes[0].dir * 10.0f);
    //   ctx.stroke();






}