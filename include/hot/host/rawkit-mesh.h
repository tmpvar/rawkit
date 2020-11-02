#pragma once

#include <rawkit/jit.h>
#include <rawkit/mesh.h>

void host_init_rawkit_mesh(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "_rawkit_mesh_ex", _rawkit_mesh_ex);
  rawkit_jit_add_export(jit, "rawkit_mesh_normal_count", rawkit_mesh_normal_count);
  rawkit_jit_add_export(jit, "rawkit_mesh_index_count", rawkit_mesh_index_count);
  rawkit_jit_add_export(jit, "rawkit_mesh_vertex_count", rawkit_mesh_vertex_count);
}
