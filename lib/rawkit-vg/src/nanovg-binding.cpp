#include "nanovg-binding.h"


void rawkit_vg_begin_frame(rawkit_vg_t *vg, VkCommandBuffer command_buffer, float windowWidth, float windowHeight, float devicePixelRatio) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }

  NVGparams *params = nvgInternalParams(ctx);
  VKNVGcontext *vk = (VKNVGcontext *)params->userPtr;

  vknvg_set_current_command_buffer(vk, command_buffer);
  nvgBeginFrame(ctx, windowWidth, windowHeight, devicePixelRatio);
}

void rawkit_vg_cancel_frame(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgCancelFrame(ctx);
}

void rawkit_vg_end_frame(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgEndFrame(ctx);
}

void rawkit_vg_global_composite_operation(rawkit_vg_t *vg, int op) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgGlobalCompositeOperation(ctx, op);
}

void rawkit_vg_global_composite_blend_func(rawkit_vg_t *vg, int sfactor, int dfactor) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgGlobalCompositeBlendFunc(ctx, sfactor, dfactor);
}

void rawkit_vg_global_composite_blend_func_separate(rawkit_vg_t *vg, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgGlobalCompositeBlendFuncSeparate(ctx, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

NVGcolor rawkit_vg_RGB(unsigned char r, unsigned char g, unsigned char b) {
  return nvgRGB(r, g, b);
}

NVGcolor rawkit_vg_RGBf(float r, float g, float b) {
  return nvgRGBf(r, g, b);
}

NVGcolor rawkit_vg_RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  return nvgRGBA(r, g, b, a);
}

NVGcolor rawkit_vg_RGBAf(float r, float g, float b, float a) {
  return nvgRGBAf(r, g, b, a);
}

NVGcolor rawkit_vg_lerpRGBA(NVGcolor c0, NVGcolor c1, float u) {
  return nvgLerpRGBA(c0, c1, u);
}

NVGcolor rawkit_vg_transRGBA(NVGcolor c0, unsigned char a) {
  return nvgTransRGBA(c0, a);
}

NVGcolor rawkit_vg_transRGBAf(NVGcolor c0, float a) {
  return nvgTransRGBAf(c0, a);
}

NVGcolor rawkit_vg_HSL(float h, float s, float l) {
  return nvgHSL(h, s, l);
}

NVGcolor rawkit_vg_HSLA(float h, float s, float l, unsigned char a) {
  return nvgHSLA(h, s, l, a);
}

void rawkit_vg_save(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgSave(ctx);
}

void rawkit_vg_restore(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgRestore(ctx);
}

void rawkit_vg_reset(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgReset(ctx);
}

void rawkit_vg_shape_anti_alias(rawkit_vg_t *vg, int enabled) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgShapeAntiAlias(ctx, enabled);
}

void rawkit_vg_stroke_color(rawkit_vg_t *vg, NVGcolor color) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgStrokeColor(ctx, color);
}

void rawkit_vg_stroke_paint(rawkit_vg_t *vg, NVGpaint paint) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgStrokePaint(ctx, paint);
}

void rawkit_vg_fill_color(rawkit_vg_t *vg, NVGcolor color) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgFillColor(ctx, color);
}

void rawkit_vg_fill_paint(rawkit_vg_t *vg, NVGpaint paint) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgFillPaint(ctx, paint);
}

void rawkit_vg_miter_limit(rawkit_vg_t *vg, float limit) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgMiterLimit(ctx, limit);
}

void rawkit_vg_stroke_width(rawkit_vg_t *vg, float size) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgStrokeWidth(ctx, size);
}

void rawkit_vg_line_cap(rawkit_vg_t *vg, int cap) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgLineCap(ctx, cap);
}

void rawkit_vg_line_join(rawkit_vg_t *vg, int join) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgLineJoin(ctx, join);
}

void rawkit_vg_global_alpha(rawkit_vg_t *vg, float alpha) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgGlobalAlpha(ctx, alpha);
}

void rawkit_vg_reset_transform(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgResetTransform(ctx);
}

void rawkit_vg_transform(rawkit_vg_t *vg, float a, float b, float c, float d, float e, float f) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgTransform(ctx, a, b, c, d, e, f);
}

void rawkit_vg_translate(rawkit_vg_t *vg, float x, float y) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgTranslate(ctx, x, y);
}

void rawkit_vg_rotate(rawkit_vg_t *vg, float angle) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgRotate(ctx, angle);
}

void rawkit_vg_skewX(rawkit_vg_t *vg, float angle) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgSkewX(ctx, angle);
}

void rawkit_vg_skewY(rawkit_vg_t *vg, float angle) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgSkewY(ctx, angle);
}

void rawkit_vg_scale(rawkit_vg_t *vg, float x, float y) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgScale(ctx, x, y);
}

void rawkit_vg_current_transform(rawkit_vg_t *vg, float*xform) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgCurrentTransform(ctx, xform);
}

void rawkit_vg_transform_identity(float*dst) {
  nvgTransformIdentity(dst);
}

void rawkit_vg_transform_translate(float*dst, float tx, float ty) {
  nvgTransformTranslate(dst, tx, ty);
}

void rawkit_vg_transform_scale(float*dst, float sx, float sy) {
  nvgTransformScale(dst, sx, sy);
}

void rawkit_vg_transform_rotate(float*dst, float a) {
  nvgTransformRotate(dst, a);
}

void rawkit_vg_transform_skewX(float*dst, float a) {
  nvgTransformSkewX(dst, a);
}

void rawkit_vg_transform_skewY(float*dst, float a) {
  nvgTransformSkewY(dst, a);
}

void rawkit_vg_transform_multiply(float*dst, const float*src) {
  nvgTransformMultiply(dst, src);
}

void rawkit_vg_transform_premultiply(float*dst, const float*src) {
  nvgTransformPremultiply(dst, src);
}

int rawkit_vg_transform_inverse(float*dst, const float*src) {
  return nvgTransformInverse(dst, src);
}

void rawkit_vg_transform_point(float*dstx, float*dsty, const float*xform, float srcx, float srcy) {
  nvgTransformPoint(dstx, dsty, xform, srcx, srcy);
}

float rawkit_vg_deg_to_rad(float deg) {
  return nvgDegToRad(deg);
}

float rawkit_vg_rad_to_deg(float rad) {
  return nvgRadToDeg(rad);
}

int rawkit_vg_create_image_rgba(rawkit_vg_t *vg, int w, int h, int imageFlags, const unsigned char* data) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgCreateImageRGBA(ctx, w, h, imageFlags, data);
}

void rawkit_vg_update_image(rawkit_vg_t *vg, int image, const unsigned char *data) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgUpdateImage(ctx, image, data);
}

void rawkit_vg_image_size(rawkit_vg_t *vg, int image, int*w, int*h) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgImageSize(ctx, image, w, h);
}

void rawkit_vg_delete_image(rawkit_vg_t *vg, int image) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgDeleteImage(ctx, image);
}

NVGpaint rawkit_vg_linear_gradient(rawkit_vg_t *vg, float sx, float sy, float ex, float ey, NVGcolor icol, NVGcolor ocol) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return NVGpaint{}; }
  return nvgLinearGradient(ctx, sx, sy, ex, ey, icol, ocol);
}

NVGpaint rawkit_vg_box_gradient(rawkit_vg_t *vg, float x, float y, float w, float h, float r, float f, NVGcolor icol, NVGcolor ocol) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return NVGpaint{}; }
  return nvgBoxGradient(ctx, x, y, w, h, r, f, icol, ocol);
}

NVGpaint rawkit_vg_radial_gradient(rawkit_vg_t *vg, float cx, float cy, float inr, float outr, NVGcolor icol, NVGcolor ocol) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return NVGpaint{}; }
  return nvgRadialGradient(ctx, cx, cy, inr, outr, icol, ocol);
}

NVGpaint rawkit_vg_image_pattern(rawkit_vg_t *vg, float ox, float oy, float ex, float ey, float angle, int image, float alpha) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return NVGpaint{}; }
  return nvgImagePattern(ctx, ox, oy, ex, ey, angle, image, alpha);
}

void rawkit_vg_scissor(rawkit_vg_t *vg, float x, float y, float w, float h) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgScissor(ctx, x, y, w, h);
}

void rawkit_vg_intersect_scissor(rawkit_vg_t *vg, float x, float y, float w, float h) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgIntersectScissor(ctx, x, y, w, h);
}

void rawkit_vg_reset_scissor(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgResetScissor(ctx);
}

void rawkit_vg_begin_path(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgBeginPath(ctx);
}

void rawkit_vg_move_to(rawkit_vg_t *vg, float x, float y) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgMoveTo(ctx, x, y);
}

void rawkit_vg_line_to(rawkit_vg_t *vg, float x, float y) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgLineTo(ctx, x, y);
}

void rawkit_vg_bezier_to(rawkit_vg_t *vg, float c1x, float c1y, float c2x, float c2y, float x, float y) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgBezierTo(ctx, c1x, c1y, c2x, c2y, x, y);
}

void rawkit_vg_quad_to(rawkit_vg_t *vg, float cx, float cy, float x, float y) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgQuadTo(ctx, cx, cy, x, y);
}

void rawkit_vg_arc_to(rawkit_vg_t *vg, float x1, float y1, float x2, float y2, float radius) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgArcTo(ctx, x1, y1, x2, y2, radius);
}

void rawkit_vg_close_path(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgClosePath(ctx);
}

void rawkit_vg_path_winding(rawkit_vg_t *vg, int dir) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgPathWinding(ctx, dir);
}

void rawkit_vg_arc(rawkit_vg_t *vg, float cx, float cy, float r, float a0, float a1, int dir) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgArc(ctx, cx, cy, r, a0, a1, dir);
}

void rawkit_vg_rect(rawkit_vg_t *vg, float x, float y, float w, float h) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgRect(ctx, x, y, w, h);
}

void rawkit_vg_rounded_rect(rawkit_vg_t *vg, float x, float y, float w, float h, float r) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgRoundedRect(ctx, x, y, w, h, r);
}

void rawkit_vg_rounded_rect_varying(rawkit_vg_t *vg, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgRoundedRectVarying(ctx, x, y, w, h, radTopLeft, radTopRight, radBottomRight, radBottomLeft);
}

void rawkit_vg_ellipse(rawkit_vg_t *vg, float cx, float cy, float rx, float ry) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgEllipse(ctx, cx, cy, rx, ry);
}

void rawkit_vg_circle(rawkit_vg_t *vg, float cx, float cy, float r) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgCircle(ctx, cx, cy, r);
}

void rawkit_vg_fill(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgFill(ctx);
}

void rawkit_vg_stroke(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgStroke(ctx);
}

int rawkit_vg_create_font(rawkit_vg_t *vg, const char *name, const char*filename) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgCreateFont(ctx, name, filename);
}

int rawkit_vg_create_font_at_index(rawkit_vg_t *vg, const char *name, const char*filename, const int fontIndex) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgCreateFontAtIndex(ctx, name, filename, fontIndex);
}

int rawkit_vg_create_font_mem(rawkit_vg_t *vg, const char *name, unsigned char*data, int ndata, int freeData) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgCreateFontMem(ctx, name, data, ndata, freeData);
}

int rawkit_vg_create_font_mem_at_index(rawkit_vg_t *vg, const char *name, unsigned char*data, int ndata, int freeData, const int fontIndex) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgCreateFontMemAtIndex(ctx, name, data, ndata, freeData, fontIndex);
}

int rawkit_vg_find_font(rawkit_vg_t *vg, const char *name) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgFindFont(ctx, name);
}

int rawkit_vg_add_fallback_font_id(rawkit_vg_t *vg, int baseFont, int fallbackFont) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgAddFallbackFontId(ctx, baseFont, fallbackFont);
}

int rawkit_vg_add_fallback_font(rawkit_vg_t *vg, const char *baseFont, const char*fallbackFont) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgAddFallbackFont(ctx, baseFont, fallbackFont);
}

void rawkit_vg_reset_fallback_fonts_id(rawkit_vg_t *vg, int baseFont) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgResetFallbackFontsId(ctx, baseFont);
}

void rawkit_vg_reset_fallback_fonts(rawkit_vg_t *vg, const char *baseFont) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgResetFallbackFonts(ctx, baseFont);
}

void rawkit_vg_font_size(rawkit_vg_t *vg, float size) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgFontSize(ctx, size);
}

void rawkit_vg_font_blur(rawkit_vg_t *vg, float blur) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgFontBlur(ctx, blur);
}

void rawkit_vg_text_letter_spacing(rawkit_vg_t *vg, float spacing) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgTextLetterSpacing(ctx, spacing);
}

void rawkit_vg_text_line_height(rawkit_vg_t *vg, float lineHeight) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgTextLineHeight(ctx, lineHeight);
}

void rawkit_vg_text_align(rawkit_vg_t *vg, int align) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgTextAlign(ctx, align);
}

void rawkit_vg_font_face_id(rawkit_vg_t *vg, int font) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgFontFaceId(ctx, font);
}

void rawkit_vg_font_face(rawkit_vg_t *vg, const char *font) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgFontFace(ctx, font);
}

float rawkit_vg_text(rawkit_vg_t *vg, float x, float y, const char *string, const char*end) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0.0f; }
  return nvgText(ctx, x, y, string, end);
}

void rawkit_vg_text_box(rawkit_vg_t *vg, float x, float y, float breakRowWidth, const char *string, const char*end) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgTextBox(ctx, x, y, breakRowWidth, string, end);
}

float rawkit_vg_text_bounds(rawkit_vg_t *vg, float x, float y, const char *string, const char*end, float*bounds) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0.0f; }
  return nvgTextBounds(ctx, x, y, string, end, bounds);
}

void rawkit_vg_text_box_bounds(rawkit_vg_t *vg, float x, float y, float breakRowWidth, const char *string, const char*end, float*bounds) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgTextBoxBounds(ctx, x, y, breakRowWidth, string, end, bounds);
}

int rawkit_vg_text_glyph_positions(rawkit_vg_t *vg, float x, float y, const char *string, const char*end, NVGglyphPosition*positions, int maxPositions) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgTextGlyphPositions(ctx, x, y, string, end, positions, maxPositions);
}

void rawkit_vg_text_metrics(rawkit_vg_t *vg, float*ascender, float*descender, float*lineh) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgTextMetrics(ctx, ascender, descender, lineh);
}

int rawkit_vg_text_break_lines(rawkit_vg_t *vg, const char *string, const char*end, float breakRowWidth, NVGtextRow*rows, int maxRows) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return 0; }
  return nvgTextBreakLines(ctx, string, end, breakRowWidth, rows, maxRows);
}

void rawkit_vg_debug_dump_path_cache(rawkit_vg_t *vg) {
  VGState *state = vg ? (VGState *)vg->_state : nullptr;
  NVGcontext *ctx = state ? state->ctx : nullptr;
  if (!ctx) { return; }
  nvgDebugDumpPathCache(ctx);
}

