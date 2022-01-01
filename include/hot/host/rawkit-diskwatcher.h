#pragma once

#include <rawkit/jit.h>
#include <rawkit/diskwatcher.h>


static void host_init_rawkit_diskwatcher(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_default_diskwatcher", rawkit_default_diskwatcher);
  rawkit_jit_add_export(jit, "_rawkit_diskwatcher_destroy", _rawkit_diskwatcher_destroy);
  rawkit_jit_add_export(jit, "_rawkit_diskwatcher_ex", _rawkit_diskwatcher_ex);
  rawkit_jit_add_export(jit, "rawkit_diskwatcher_file_version", rawkit_diskwatcher_file_version);
}
