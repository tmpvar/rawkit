#pragma once

#include <rawkit/jit.h>
#include <rawkit/diskwatcher.h>


void host_init_rawkit_diskwatcher(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_default_diskwatcher", (void *)&rawkit_default_diskwatcher);
  rawkit_jit_add_export(jit, "_rawkit_diskwatcher_destroy", (void *)&_rawkit_diskwatcher_destroy);
  rawkit_jit_add_export(jit, "_rawkit_diskwatcher_ex", (void *)&_rawkit_diskwatcher_ex);
  rawkit_jit_add_export(jit, "rawkit_diskwatcher_file_version", (void *)&rawkit_diskwatcher_file_version);
}
