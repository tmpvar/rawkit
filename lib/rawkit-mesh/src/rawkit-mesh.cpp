#include <rawkit/mesh.h>
#include <rawkit/file.h>
#include <rawkit/hot.h>

#include "stl.h"

#include <string>
using namespace std;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

uint32_t rawkit_mesh_normal_count(const rawkit_mesh_t *mesh) {
  if (!mesh || !mesh->normal_data) {
    return 0;
  }

  return sb_count(mesh->normal_data) / 3;
}

uint32_t rawkit_mesh_index_count(const rawkit_mesh_t *mesh) {
  if (!mesh || !mesh->index_data) {
    return 0;
  }

  return sb_count(mesh->index_data);
}

uint32_t rawkit_mesh_vertex_count(const rawkit_mesh_t *mesh) {
  if (!mesh || !mesh->vertex_data) {
    return 0;
  }

  return sb_count(mesh->vertex_data) / 3;
}

rawkit_mesh_t *_rawkit_mesh_ex(
  const char *from_file,
  const char *path,
  uv_loop_t *loop,
  rawkit_diskwatcher_t *watcher
) {
  string id = string("file+rawkit-mesh://") + path + " from " + from_file;

  const rawkit_file_t *file = _rawkit_file_ex(from_file, path, loop, watcher);

  if (!file || file->error) {
    return NULL;
  }

  rawkit_mesh_t *mesh = (rawkit_mesh_t *)calloc(sizeof(rawkit_mesh_t), 1);
  if (!mesh) {
    return NULL;
  }

  fs::path p(path);
  if (p.extension() == ".stl") {
    if (file->len < 80) {
      return NULL;
    }

    // Note: this does not account for degenerate cases where someone
    // has added the description `solid` at the start of their binary stl...
    if (strncmp((const char *)file->data, "solid ", 6) == 0) {
      stl_ascii_init(file, mesh);
    } else {
      stl_binary_init(file, mesh);
    }
    return mesh;
  }

  printf("ERROR: unhandled extension (%s)\n", p.extension().string().c_str());
  return NULL;
}
