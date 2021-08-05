
#pragma once
#include <rawkit/jit.h>
#include <hidapi/hidapi.h>

void host_init_hidapi(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "hid_init", hid_init);
  rawkit_jit_add_export(jit, "hid_exit", hid_exit);
  rawkit_jit_add_export(jit, "hid_enumerate", hid_enumerate);
  rawkit_jit_add_export(jit, "hid_free_enumeration", hid_free_enumeration);
  rawkit_jit_add_export(jit, "hid_open", hid_open);
  rawkit_jit_add_export(jit, "hid_open_path", hid_open_path);
  rawkit_jit_add_export(jit, "hid_write", hid_write);
  rawkit_jit_add_export(jit, "hid_read_timeout", hid_read_timeout);
  rawkit_jit_add_export(jit, "hid_read", hid_read);
  rawkit_jit_add_export(jit, "hid_set_nonblocking", hid_set_nonblocking);
  rawkit_jit_add_export(jit, "hid_send_feature_report", hid_send_feature_report);
  rawkit_jit_add_export(jit, "hid_get_feature_report", hid_get_feature_report);
  rawkit_jit_add_export(jit, "hid_get_input_report", hid_get_input_report);
  rawkit_jit_add_export(jit, "hid_close", hid_close);
  rawkit_jit_add_export(jit, "hid_get_manufacturer_string", hid_get_manufacturer_string);
  rawkit_jit_add_export(jit, "hid_get_product_string", hid_get_product_string);
  rawkit_jit_add_export(jit, "hid_get_serial_number_string", hid_get_serial_number_string);
  rawkit_jit_add_export(jit, "hid_get_indexed_string", hid_get_indexed_string);
  rawkit_jit_add_export(jit, "hid_error", hid_error);
  rawkit_jit_add_export(jit, "hid_version", hid_version);
  rawkit_jit_add_export(jit, "hid_version_str", hid_version_str);
}
