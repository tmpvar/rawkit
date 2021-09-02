
#pragma once
#include <rawkit/jit.h>
#include <pull/stream.h>

static void host_init_pull_stream(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "create_file_source", create_file_source);
  rawkit_jit_add_export(jit, "create_file_sink", create_file_sink);
  rawkit_jit_add_export(jit, "ps_uv_alloc_cb", ps_uv_alloc_cb);
  rawkit_jit_add_export(jit, "ps_uv_read_cb", ps_uv_read_cb);
  rawkit_jit_add_export(jit, "ps_uv_write_cb", ps_uv_write_cb);
  rawkit_jit_add_export(jit, "ps_uv_write", ps_uv_write);
  rawkit_jit_add_export(jit, "ps_uv_sink_fn", ps_uv_sink_fn);
  rawkit_jit_add_export(jit, "ps_uv_source_fn", ps_uv_source_fn);
  rawkit_jit_add_export(jit, "create_tcp_client", create_tcp_client);
  rawkit_jit_add_export(jit, "create_tcp_client_from_stream", create_tcp_client_from_stream);
  rawkit_jit_add_export(jit, "create_tcp_server", create_tcp_server);
  rawkit_jit_add_export(jit, "_ps_status", _ps_status);
  rawkit_jit_add_export(jit, "ps_pull", ps_pull);
  rawkit_jit_add_export(jit, "ps__pull_from_source", ps__pull_from_source);
  rawkit_jit_add_export(jit, "_ps_destroy", _ps_destroy);
  rawkit_jit_add_export(jit, "_ps_create", _ps_create);
  rawkit_jit_add_export(jit, "_ps_pipeline", _ps_pipeline);
  rawkit_jit_add_export(jit, "create_counter", create_counter);
  rawkit_jit_add_export(jit, "create_multiplier", create_multiplier);
  rawkit_jit_add_export(jit, "create_nooper", create_nooper);
  rawkit_jit_add_export(jit, "create_taker", create_taker);
  rawkit_jit_add_export(jit, "create_collector", create_collector);
  rawkit_jit_add_export(jit, "create_hex_printer", create_hex_printer);
  rawkit_jit_add_export(jit, "create_single_value", create_single_value);
  rawkit_jit_add_export(jit, "create_reverser", create_reverser);
  rawkit_jit_add_export(jit, "create_splitter", create_splitter);
  rawkit_jit_add_export(jit, "create_user_value", create_user_value);
  rawkit_jit_add_export(jit, "ps_user_value_from_str", ps_user_value_from_str);
}

