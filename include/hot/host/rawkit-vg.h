#pragma once
#include <rawkit/jit.h>
#include <rawkit/vg.h>

void host_init_rawkit_vg(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_vg", rawkit_vg);
  rawkit_jit_add_export(jit, "rawkit_default_vg", rawkit_default_vg);
  rawkit_jit_add_export(jit, "rawkit_vg_texture", rawkit_vg_texture);
  rawkit_jit_add_export(jit, "rawkit_vg_draw_texture", rawkit_vg_draw_texture);
  rawkit_jit_add_export(jit, "rawkit_vg_draw_texture_rect", rawkit_vg_draw_texture_rect);
  rawkit_jit_add_export(jit, "rawkit_vg_from_gpu_texture_target", rawkit_vg_from_gpu_texture_target);

  rawkit_jit_add_export(jit, "rawkit_vg_begin_frame", rawkit_vg_begin_frame);
  rawkit_jit_add_export(jit, "rawkit_vg_cancel_frame", rawkit_vg_cancel_frame);
  rawkit_jit_add_export(jit, "rawkit_vg_end_frame", rawkit_vg_end_frame);
  rawkit_jit_add_export(jit, "rawkit_vg_global_composite_operation", rawkit_vg_global_composite_operation);
  rawkit_jit_add_export(jit, "rawkit_vg_global_composite_blend_func", rawkit_vg_global_composite_blend_func);
  rawkit_jit_add_export(jit, "rawkit_vg_global_composite_blend_func_separate", rawkit_vg_global_composite_blend_func_separate);
  rawkit_jit_add_export(jit, "rawkit_vg_RGB", rawkit_vg_RGB);
  rawkit_jit_add_export(jit, "rawkit_vg_RGBf", rawkit_vg_RGBf);
  rawkit_jit_add_export(jit, "rawkit_vg_RGBA", rawkit_vg_RGBA);
  rawkit_jit_add_export(jit, "rawkit_vg_RGBAf", rawkit_vg_RGBAf);
  rawkit_jit_add_export(jit, "rawkit_vg_lerpRGBA", rawkit_vg_lerpRGBA);
  rawkit_jit_add_export(jit, "rawkit_vg_transRGBA", rawkit_vg_transRGBA);
  rawkit_jit_add_export(jit, "rawkit_vg_transRGBAf", rawkit_vg_transRGBAf);
  rawkit_jit_add_export(jit, "rawkit_vg_HSL", rawkit_vg_HSL);
  rawkit_jit_add_export(jit, "rawkit_vg_HSLA", rawkit_vg_HSLA);
  rawkit_jit_add_export(jit, "rawkit_vg_save", rawkit_vg_save);
  rawkit_jit_add_export(jit, "rawkit_vg_restore", rawkit_vg_restore);
  rawkit_jit_add_export(jit, "rawkit_vg_reset", rawkit_vg_reset);
  rawkit_jit_add_export(jit, "rawkit_vg_shape_anti_alias", rawkit_vg_shape_anti_alias);
  rawkit_jit_add_export(jit, "rawkit_vg_stroke_color", rawkit_vg_stroke_color);
  rawkit_jit_add_export(jit, "rawkit_vg_stroke_paint", rawkit_vg_stroke_paint);
  rawkit_jit_add_export(jit, "rawkit_vg_fill_color", rawkit_vg_fill_color);
  rawkit_jit_add_export(jit, "rawkit_vg_fill_paint", rawkit_vg_fill_paint);
  rawkit_jit_add_export(jit, "rawkit_vg_miter_limit", rawkit_vg_miter_limit);
  rawkit_jit_add_export(jit, "rawkit_vg_stroke_width", rawkit_vg_stroke_width);
  rawkit_jit_add_export(jit, "rawkit_vg_line_cap", rawkit_vg_line_cap);
  rawkit_jit_add_export(jit, "rawkit_vg_line_join", rawkit_vg_line_join);
  rawkit_jit_add_export(jit, "rawkit_vg_global_alpha", rawkit_vg_global_alpha);
  rawkit_jit_add_export(jit, "rawkit_vg_reset_transform", rawkit_vg_reset_transform);
  rawkit_jit_add_export(jit, "rawkit_vg_transform", rawkit_vg_transform);
  rawkit_jit_add_export(jit, "rawkit_vg_translate", rawkit_vg_translate);
  rawkit_jit_add_export(jit, "rawkit_vg_rotate", rawkit_vg_rotate);
  rawkit_jit_add_export(jit, "rawkit_vg_skewX", rawkit_vg_skewX);
  rawkit_jit_add_export(jit, "rawkit_vg_skewY", rawkit_vg_skewY);
  rawkit_jit_add_export(jit, "rawkit_vg_scale", rawkit_vg_scale);
  rawkit_jit_add_export(jit, "rawkit_vg_current_transform", rawkit_vg_current_transform);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_identity", rawkit_vg_transform_identity);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_translate", rawkit_vg_transform_translate);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_scale", rawkit_vg_transform_scale);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_rotate", rawkit_vg_transform_rotate);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_skewX", rawkit_vg_transform_skewX);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_skewY", rawkit_vg_transform_skewY);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_multiply", rawkit_vg_transform_multiply);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_premultiply", rawkit_vg_transform_premultiply);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_inverse", rawkit_vg_transform_inverse);
  rawkit_jit_add_export(jit, "rawkit_vg_transform_point", rawkit_vg_transform_point);
  rawkit_jit_add_export(jit, "rawkit_vg_deg_to_rad", rawkit_vg_deg_to_rad);
  rawkit_jit_add_export(jit, "rawkit_vg_rad_to_deg", rawkit_vg_rad_to_deg);
  rawkit_jit_add_export(jit, "rawkit_vg_create_image_rgba", rawkit_vg_create_image_rgba);
  rawkit_jit_add_export(jit, "rawkit_vg_update_image", rawkit_vg_update_image);
  rawkit_jit_add_export(jit, "rawkit_vg_image_size", rawkit_vg_image_size);
  rawkit_jit_add_export(jit, "rawkit_vg_delete_image", rawkit_vg_delete_image);
  rawkit_jit_add_export(jit, "rawkit_vg_linear_gradient", rawkit_vg_linear_gradient);
  rawkit_jit_add_export(jit, "rawkit_vg_box_gradient", rawkit_vg_box_gradient);
  rawkit_jit_add_export(jit, "rawkit_vg_radial_gradient", rawkit_vg_radial_gradient);
  rawkit_jit_add_export(jit, "rawkit_vg_image_pattern", rawkit_vg_image_pattern);
  rawkit_jit_add_export(jit, "rawkit_vg_scissor", rawkit_vg_scissor);
  rawkit_jit_add_export(jit, "rawkit_vg_intersect_scissor", rawkit_vg_intersect_scissor);
  rawkit_jit_add_export(jit, "rawkit_vg_reset_scissor", rawkit_vg_reset_scissor);
  rawkit_jit_add_export(jit, "rawkit_vg_begin_path", rawkit_vg_begin_path);
  rawkit_jit_add_export(jit, "rawkit_vg_move_to", rawkit_vg_move_to);
  rawkit_jit_add_export(jit, "rawkit_vg_line_to", rawkit_vg_line_to);
  rawkit_jit_add_export(jit, "rawkit_vg_bezier_to", rawkit_vg_bezier_to);
  rawkit_jit_add_export(jit, "rawkit_vg_quad_to", rawkit_vg_quad_to);
  rawkit_jit_add_export(jit, "rawkit_vg_arc_to", rawkit_vg_arc_to);
  rawkit_jit_add_export(jit, "rawkit_vg_close_path", rawkit_vg_close_path);
  rawkit_jit_add_export(jit, "rawkit_vg_path_winding", rawkit_vg_path_winding);
  rawkit_jit_add_export(jit, "rawkit_vg_arc", rawkit_vg_arc);
  rawkit_jit_add_export(jit, "rawkit_vg_rect", rawkit_vg_rect);
  rawkit_jit_add_export(jit, "rawkit_vg_rounded_rect", rawkit_vg_rounded_rect);
  rawkit_jit_add_export(jit, "rawkit_vg_rounded_rect_varying", rawkit_vg_rounded_rect_varying);
  rawkit_jit_add_export(jit, "rawkit_vg_ellipse", rawkit_vg_ellipse);
  rawkit_jit_add_export(jit, "rawkit_vg_circle", rawkit_vg_circle);
  rawkit_jit_add_export(jit, "rawkit_vg_fill", rawkit_vg_fill);
  rawkit_jit_add_export(jit, "rawkit_vg_stroke", rawkit_vg_stroke);
  rawkit_jit_add_export(jit, "rawkit_vg_create_font", rawkit_vg_create_font);
  rawkit_jit_add_export(jit, "rawkit_vg_create_font_at_index", rawkit_vg_create_font_at_index);
  rawkit_jit_add_export(jit, "rawkit_vg_create_font_mem", rawkit_vg_create_font_mem);
  rawkit_jit_add_export(jit, "rawkit_vg_create_font_mem_at_index", rawkit_vg_create_font_mem_at_index);
  rawkit_jit_add_export(jit, "rawkit_vg_find_font", rawkit_vg_find_font);
  rawkit_jit_add_export(jit, "rawkit_vg_add_fallback_font_id", rawkit_vg_add_fallback_font_id);
  rawkit_jit_add_export(jit, "rawkit_vg_add_fallback_font", rawkit_vg_add_fallback_font);
  rawkit_jit_add_export(jit, "rawkit_vg_reset_fallback_fonts_id", rawkit_vg_reset_fallback_fonts_id);
  rawkit_jit_add_export(jit, "rawkit_vg_reset_fallback_fonts", rawkit_vg_reset_fallback_fonts);
  rawkit_jit_add_export(jit, "rawkit_vg_font_size", rawkit_vg_font_size);
  rawkit_jit_add_export(jit, "rawkit_vg_font_blur", rawkit_vg_font_blur);
  rawkit_jit_add_export(jit, "rawkit_vg_text_letter_spacing", rawkit_vg_text_letter_spacing);
  rawkit_jit_add_export(jit, "rawkit_vg_text_line_height", rawkit_vg_text_line_height);
  rawkit_jit_add_export(jit, "rawkit_vg_text_align", rawkit_vg_text_align);
  rawkit_jit_add_export(jit, "rawkit_vg_font_face_id", rawkit_vg_font_face_id);
  rawkit_jit_add_export(jit, "rawkit_vg_font_face", rawkit_vg_font_face);
  rawkit_jit_add_export(jit, "rawkit_vg_text", rawkit_vg_text);
  rawkit_jit_add_export(jit, "rawkit_vg_text_box", rawkit_vg_text_box);
  rawkit_jit_add_export(jit, "rawkit_vg_text_bounds", rawkit_vg_text_bounds);
  rawkit_jit_add_export(jit, "rawkit_vg_text_box_bounds", rawkit_vg_text_box_bounds);
  rawkit_jit_add_export(jit, "rawkit_vg_text_glyph_positions", rawkit_vg_text_glyph_positions);
  rawkit_jit_add_export(jit, "rawkit_vg_text_metrics", rawkit_vg_text_metrics);
  rawkit_jit_add_export(jit, "rawkit_vg_text_break_lines", rawkit_vg_text_break_lines);
  rawkit_jit_add_export(jit, "rawkit_vg_debug_dump_path_cache", rawkit_vg_debug_dump_path_cache);
}
