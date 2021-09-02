#pragma once

#include <rawkit/jit.h>

static void host_init_rawkit_jit(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_jit_create", rawkit_jit_create);
  rawkit_jit_add_export(jit, "rawkit_jit_get_status", rawkit_jit_get_status);
  rawkit_jit_add_export(jit, "_rawkit_jit_destroy", _rawkit_jit_destroy);
  rawkit_jit_add_export(jit, "rawkit_jit_program_path", rawkit_jit_program_path);
  rawkit_jit_add_export(jit, "_rawkit_jit_add_export", _rawkit_jit_add_export);
  rawkit_jit_add_export(jit, "rawkit_jit_get_message", rawkit_jit_get_message);
  rawkit_jit_add_export(jit, "rawkit_jit_call_setup", rawkit_jit_call_setup);
  rawkit_jit_add_export(jit, "rawkit_jit_call_loop", rawkit_jit_call_loop);
  rawkit_jit_add_export(jit, "rawkit_jit_version", rawkit_jit_version);
  rawkit_jit_add_export(jit, "rawkit_set_default_jit", rawkit_set_default_jit);
  rawkit_jit_add_export(jit, "rawkit_default_jit", rawkit_default_jit);
  rawkit_jit_add_export(jit, "rawkit_teardown_fn_ex", rawkit_teardown_fn_ex);
  rawkit_jit_add_export(jit, "rawkit_jit_set_debug", rawkit_jit_set_debug);
  rawkit_jit_add_export(jit, "rawkit_jit_get_version", rawkit_jit_get_version);
  rawkit_jit_add_export(jit, "rawkit_jit_add_define", rawkit_jit_add_define);
}
