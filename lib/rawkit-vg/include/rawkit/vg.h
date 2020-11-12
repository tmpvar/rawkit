#pragma once

#include <rawkit/core.h>
#include <rawkit/hot.h>
#include <nanovg/nanovg.h>
#include <rawkit/vulkan.h>

typedef struct rawkit_vg_t {

  // internal
  void *_state;
} rawkit_vg_t;

#ifdef __cplusplus
extern "C" {
#endif

  rawkit_vg_t *rawkit_vg(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkRenderPass render_pass
  );

  rawkit_vg_t *rawkit_vg_default();

  void rawkit_vg_begin_frame(rawkit_vg_t *vg, float windowWidth, float windowHeight, float devicePixelRatio);
  void rawkit_vg_cancel_frame(rawkit_vg_t *vg);
  void rawkit_vg_end_frame(rawkit_vg_t *vg);
  void rawkit_vg_global_composite_operation(rawkit_vg_t *vg, int op);
  void rawkit_vg_global_composite_blend_func(rawkit_vg_t *vg, int sfactor, int dfactor);
  void rawkit_vg_global_composite_blend_func_separate(rawkit_vg_t *vg, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);
  NVGcolor rawkit_vg_RGB(unsigned char r, unsigned char g, unsigned char b);
  NVGcolor rawkit_vg_RGBf(float r, float g, float b);
  NVGcolor rawkit_vg_RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  NVGcolor rawkit_vg_RGBAf(float r, float g, float b, float a);
  NVGcolor rawkit_vg_lerpRGBA(NVGcolor c0, NVGcolor c1, float u);
  NVGcolor rawkit_vg_transRGBA(NVGcolor c0, unsigned char a);
  NVGcolor rawkit_vg_transRGBAf(NVGcolor c0, float a);
  NVGcolor rawkit_vg_HSL(float h, float s, float l);
  NVGcolor rawkit_vg_HSLA(float h, float s, float l, unsigned char a);
  void rawkit_vg_save(rawkit_vg_t *vg);
  void rawkit_vg_restore(rawkit_vg_t *vg);
  void rawkit_vg_reset(rawkit_vg_t *vg);
  void rawkit_vg_shape_anti_alias(rawkit_vg_t *vg, int enabled);
  void rawkit_vg_stroke_color(rawkit_vg_t *vg, NVGcolor color);
  void rawkit_vg_stroke_paint(rawkit_vg_t *vg, NVGpaint paint);
  void rawkit_vg_fill_color(rawkit_vg_t *vg, NVGcolor color);
  void rawkit_vg_fill_paint(rawkit_vg_t *vg, NVGpaint paint);
  void rawkit_vg_miter_limit(rawkit_vg_t *vg, float limit);
  void rawkit_vg_stroke_width(rawkit_vg_t *vg, float size);
  void rawkit_vg_line_cap(rawkit_vg_t *vg, int cap);
  void rawkit_vg_line_join(rawkit_vg_t *vg, int join);
  void rawkit_vg_global_alpha(rawkit_vg_t *vg, float alpha);
  void rawkit_vg_reset_transform(rawkit_vg_t *vg);
  void rawkit_vg_transform(rawkit_vg_t *vg, float a, float b, float c, float d, float e, float f);
  void rawkit_vg_translate(rawkit_vg_t *vg, float x, float y);
  void rawkit_vg_rotate(rawkit_vg_t *vg, float angle);
  void rawkit_vg_skewX(rawkit_vg_t *vg, float angle);
  void rawkit_vg_skewY(rawkit_vg_t *vg, float angle);
  void rawkit_vg_scale(rawkit_vg_t *vg, float x, float y);
  void rawkit_vg_current_transform(rawkit_vg_t *vg, float*xform);
  void rawkit_vg_transform_identity(float*dst);
  void rawkit_vg_transform_translate(float*dst, float tx, float ty);
  void rawkit_vg_transform_scale(float*dst, float sx, float sy);
  void rawkit_vg_transform_rotate(float*dst, float a);
  void rawkit_vg_transform_skewX(float*dst, float a);
  void rawkit_vg_transform_skewY(float*dst, float a);
  void rawkit_vg_transform_multiply(float*dst, const float*src);
  void rawkit_vg_transform_premultiply(float*dst, const float*src);
  int rawkit_vg_transform_inverse(float*dst, const float*src);
  void rawkit_vg_transform_point(float*dstx, float*dsty, const float*xform, float srcx, float srcy);
  float rawkit_vg_deg_to_rad(float deg);
  float rawkit_vg_rad_to_deg(float rad);
  void rawkit_vg_update_image(rawkit_vg_t *vg, int image, const unsigned char *data);
  void rawkit_vg_image_size(rawkit_vg_t *vg, int image, int*w, int*h);
  void rawkit_vg_delete_image(rawkit_vg_t *vg, int image);
  NVGpaint rawkit_vg_linear_gradient(rawkit_vg_t *vg, float sx, float sy, float ex, float ey, NVGcolor icol, NVGcolor ocol);
  NVGpaint rawkit_vg_box_gradient(rawkit_vg_t *vg, float x, float y, float w, float h, float r, float f, NVGcolor icol, NVGcolor ocol);
  NVGpaint rawkit_vg_radial_gradient(rawkit_vg_t *vg, float cx, float cy, float inr, float outr, NVGcolor icol, NVGcolor ocol);
  NVGpaint rawkit_vg_image_pattern(rawkit_vg_t *vg, float ox, float oy, float ex, float ey, float angle, int image, float alpha);
  void rawkit_vg_scissor(rawkit_vg_t *vg, float x, float y, float w, float h);
  void rawkit_vg_intersect_scissor(rawkit_vg_t *vg, float x, float y, float w, float h);
  void rawkit_vg_reset_scissor(rawkit_vg_t *vg);
  void rawkit_vg_begin_path(rawkit_vg_t *vg);
  void rawkit_vg_move_to(rawkit_vg_t *vg, float x, float y);
  void rawkit_vg_line_to(rawkit_vg_t *vg, float x, float y);
  void rawkit_vg_bezier_to(rawkit_vg_t *vg, float c1x, float c1y, float c2x, float c2y, float x, float y);
  void rawkit_vg_quad_to(rawkit_vg_t *vg, float cx, float cy, float x, float y);
  void rawkit_vg_arc_to(rawkit_vg_t *vg, float x1, float y1, float x2, float y2, float radius);
  void rawkit_vg_close_path(rawkit_vg_t *vg);
  void rawkit_vg_path_winding(rawkit_vg_t *vg, int dir);
  void rawkit_vg_arc(rawkit_vg_t *vg, float cx, float cy, float r, float a0, float a1, int dir);
  void rawkit_vg_rect(rawkit_vg_t *vg, float x, float y, float w, float h);
  void rawkit_vg_rounded_rect(rawkit_vg_t *vg, float x, float y, float w, float h, float r);
  void rawkit_vg_rounded_rect_varying(rawkit_vg_t *vg, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);
  void rawkit_vg_ellipse(rawkit_vg_t *vg, float cx, float cy, float rx, float ry);
  void rawkit_vg_circle(rawkit_vg_t *vg, float cx, float cy, float r);
  void rawkit_vg_fill(rawkit_vg_t *vg);
  void rawkit_vg_stroke(rawkit_vg_t *vg);
  int rawkit_vg_create_font(rawkit_vg_t *vg, const char *name, const char*filename);
  int rawkit_vg_create_font_at_index(rawkit_vg_t *vg, const char *name, const char*filename, const int fontIndex);
  int rawkit_vg_create_font_mem(rawkit_vg_t *vg, const char *name, unsigned char*data, int ndata, int freeData);
  int rawkit_vg_create_font_mem_at_index(rawkit_vg_t *vg, const char *name, unsigned char*data, int ndata, int freeData, const int fontIndex);
  int rawkit_vg_find_font(rawkit_vg_t *vg, const char *name);
  int rawkit_vg_add_fallback_font_id(rawkit_vg_t *vg, int baseFont, int fallbackFont);
  int rawkit_vg_add_fallback_font(rawkit_vg_t *vg, const char *baseFont, const char*fallbackFont);
  void rawkit_vg_reset_fallback_fonts_id(rawkit_vg_t *vg, int baseFont);
  void rawkit_vg_reset_fallback_fonts(rawkit_vg_t *vg, const char *baseFont);
  void rawkit_vg_font_size(rawkit_vg_t *vg, float size);
  void rawkit_vg_font_blur(rawkit_vg_t *vg, float blur);
  void rawkit_vg_text_letter_spacing(rawkit_vg_t *vg, float spacing);
  void rawkit_vg_text_line_height(rawkit_vg_t *vg, float lineHeight);
  void rawkit_vg_text_align(rawkit_vg_t *vg, int align);
  void rawkit_vg_font_face_id(rawkit_vg_t *vg, int font);
  void rawkit_vg_font_face(rawkit_vg_t *vg, const char *font);
  float rawkit_vg_text(rawkit_vg_t *vg, float x, float y, const char *string, const char*end);
  void rawkit_vg_text_box(rawkit_vg_t *vg, float x, float y, float breakRowWidth, const char *string, const char*end);
  float rawkit_vg_text_bounds(rawkit_vg_t *vg, float x, float y, const char *string, const char*end, float*bounds);
  void rawkit_vg_text_box_bounds(rawkit_vg_t *vg, float x, float y, float breakRowWidth, const char *string, const char*end, float*bounds);
  int rawkit_vg_text_glyph_positions(rawkit_vg_t *vg, float x, float y, const char *string, const char*end, NVGglyphPosition*positions, int maxPositions);
  void rawkit_vg_text_metrics(rawkit_vg_t *vg, float*ascender, float*descender, float*lineh);
  int rawkit_vg_text_break_lines(rawkit_vg_t *vg, const char *string, const char*end, float breakRowWidth, NVGtextRow*rows, int maxRows);
  void rawkit_vg_debug_dump_path_cache(rawkit_vg_t *vg);
#ifdef __cplusplus
}
#endif
