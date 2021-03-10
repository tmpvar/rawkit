#pragma once

#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
using namespace glm;

class Color {
  public:
    NVGcolor value;
};

Color rgbf(float r, float g, float b, float a = 1.0f) {
  Color ret = {
    .value = rawkit_vg_RGBAf(r, g, b, a)
  };
  return ret;
}

Color rgb(uint32_t r, uint32_t g, uint32_t b, uint32_t a = 255) {
  Color ret = {
    .value = rawkit_vg_RGBA(r, g, b, a)
  };
  return ret;
}

Color rgb(uint32_t packed) {
  Color ret = {
    .value = rawkit_vg_RGBA(
      (packed >> 24) & 0xFF,
      (packed >> 16) & 0xFF,
      (packed >> 8) & 0xFF,
      (packed >> 0) & 0xFF
    )
  };
  return ret;
}

Color hsl(float h, float s, float l, float a = 1.0f) {
  Color ret = {
    .value = rawkit_vg_HSLA(h, s, l, (uint8_t)(a * 255.0f))
  };
  return ret;
}

class Context2D {
  public:
    rawkit_vg_t *vg;
    Context2D() {
      this->vg = rawkit_default_vg();
    }

    Context2D *beginPath() {
      rawkit_vg_begin_path(vg);
      return this;
    }

    Context2D *closePath() {
      rawkit_vg_close_path(vg);
      return this;
    }

    Context2D *save() {
      rawkit_vg_save(vg);
      return this;
    }

    Context2D *restore() {
      rawkit_vg_restore(vg);
      return this;
    }

    Context2D *translate(vec2 offset) {
      rawkit_vg_translate(vg, offset.x, offset.y);
      return this;
    }

    Context2D *rotate(float rads) {
      rawkit_vg_rotate(vg, rads);
      return this;
    }

    Context2D *scale(vec2 amount) {
      rawkit_vg_scale(vg, amount.x, amount.y);
      return this;
    }

    Context2D *moveTo(glm::vec2 p) {
      rawkit_vg_move_to(vg, p.x, p.y);
      return this;
    }

    Context2D *lineTo(glm::vec2 p) {
      rawkit_vg_line_to(vg, p.x, p.y);
      return this;
    }

    Context2D *arc(
      glm::vec2 center,
      float radius,
      float startAngle = 0.0,
      float endAngle = 6.283185307179586,
      int dir = 1
    ) {
      rawkit_vg_arc(vg, center.x, center.y, radius, startAngle, endAngle, dir);
      return this;
    }

    Context2D *fill() {
      rawkit_vg_fill(vg);
      return this;
    }

    Context2D *stroke() {
      rawkit_vg_stroke(vg);
      return this;
    }


    Context2D *strokeWidth(float v) {
      rawkit_vg_stroke_width(vg, v);
      return this;
    }

    Context2D *strokeColor(Color col) {
      rawkit_vg_stroke_color(vg, col.value);
      return this;
    }

    Context2D *fillColor(Color col) {
      rawkit_vg_fill_color(vg, col.value);
      return this;
    }

    Context2D *drawTexture(vec2 pos, vec2 dims, rawkit_texture_t *tex, rawkit_texture_sampler_t *sampler = nullptr) {
      if (!tex) {
        return this;
      }
      const rawkit_texture_sampler_t *actual_sampler = (sampler == nullptr)
        ? tex->default_sampler
        : sampler;

      rawkit_vg_draw_texture(
        vg,
        pos.x,
        pos.y,
        dims.x,
        dims.y,
        tex,
        actual_sampler
      );
      return this;
    }

    Context2D *rect(vec2 pos, vec2 dims) {
      rawkit_vg_rect(
        vg,
        pos.x,
        pos.y,
        dims.x,
        dims.y
      );
      return this;
    }
};