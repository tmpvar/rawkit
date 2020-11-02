#pragma once

#include <uv.h>
#include <rawkit/diskwatcher.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rawkit_mesh_t {
  uint32_t *normal_data;
  uint32_t *index_data;
  float *vertex_data;
} rawkit_mesh_t;


uint32_t rawkit_mesh_normal_count(const rawkit_mesh_t *mesh);
uint32_t rawkit_mesh_index_count(const rawkit_mesh_t *mesh);
uint32_t rawkit_mesh_vertex_count(const rawkit_mesh_t *mesh);

rawkit_mesh_t *_rawkit_mesh_ex(
  const char *from_file,
  const char *path,
  uv_loop_t *loop,
  rawkit_diskwatcher_t *watcher
);

#define rawkit_mesh_ex(path, loop, diskwatcher) _rawkit_mesh_ex(__FILE__, path, loop, diskwatcher)
#define rawkit_mesh(path) _rawkit_mesh_ex(__FILE__, path, uv_default_loop(), rawkit_default_diskwatcher())

#ifdef __cplusplus
}
#endif