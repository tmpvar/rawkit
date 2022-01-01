#pragma once

#include <rawkit/jit.h>
#include <rawkit/core.h>

static void host_init_rawkit_core(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "_rawkit_resource_sources", _rawkit_resource_sources);
  rawkit_jit_add_export(jit, "rawkit_resource_sources_array", rawkit_resource_sources_array);
  rawkit_jit_add_export(jit, "rawkit_now", rawkit_now);
  rawkit_jit_add_export(jit, "rawkit_randf", rawkit_randf);

  rawkit_jit_add_export(jit, "rawkit_arg_pos_count", rawkit_arg_pos_count);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_bool", rawkit_arg_pos_bool);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_i8", rawkit_arg_pos_i8);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_u8", rawkit_arg_pos_u8);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_i16", rawkit_arg_pos_i16);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_u16", rawkit_arg_pos_u16);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_i32", rawkit_arg_pos_i32);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_u32", rawkit_arg_pos_u32);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_i64", rawkit_arg_pos_i64);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_u64", rawkit_arg_pos_u64);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_f32", rawkit_arg_pos_f32);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_f64", rawkit_arg_pos_f64);
  rawkit_jit_add_export(jit, "rawkit_arg_pos_string", rawkit_arg_pos_string);

  rawkit_jit_add_export(jit, "rawkit_arg_bool", rawkit_arg_bool);
  rawkit_jit_add_export(jit, "rawkit_arg_i8", rawkit_arg_i8);
  rawkit_jit_add_export(jit, "rawkit_arg_u8", rawkit_arg_u8);
  rawkit_jit_add_export(jit, "rawkit_arg_i16", rawkit_arg_i16);
  rawkit_jit_add_export(jit, "rawkit_arg_u16", rawkit_arg_u16);
  rawkit_jit_add_export(jit, "rawkit_arg_i32", rawkit_arg_i32);
  rawkit_jit_add_export(jit, "rawkit_arg_u32", rawkit_arg_u32);
  rawkit_jit_add_export(jit, "rawkit_arg_i64", rawkit_arg_i64);
  rawkit_jit_add_export(jit, "rawkit_arg_u64", rawkit_arg_u64);
  rawkit_jit_add_export(jit, "rawkit_arg_f32", rawkit_arg_f32);
  rawkit_jit_add_export(jit, "rawkit_arg_f64", rawkit_arg_f64);
  rawkit_jit_add_export(jit, "rawkit_arg_string", rawkit_arg_string);

  rawkit_jit_add_export(jit, "rawkit_randf", rawkit_randf);
}