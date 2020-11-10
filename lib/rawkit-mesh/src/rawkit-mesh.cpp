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

static const char *resource_name = "rawkit::mesh";

rawkit_mesh_t *_rawkit_mesh_ex(
  const char *from_file,
  const char *path,
  uv_loop_t *loop,
  rawkit_diskwatcher_t *watcher
) {
  const rawkit_file_t *file = _rawkit_file_ex(from_file, path, loop, watcher);

  uint64_t id = rawkit_hash_resources(resource_name, 1, (const rawkit_resource_t **)&file);
  rawkit_mesh_t *mesh = rawkit_hot_resource_id(resource_name, id, rawkit_mesh_t);
  if (!mesh) {
    return NULL;
  }

  if (!file || file->error) {
    return mesh;
  }

  bool dirty = rawkit_resource_sources(mesh, file);
  if (!dirty) {
    return mesh;
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

    mesh->resource_version++;
    return mesh;
  }

  printf("ERROR: unhandled extension (%s)\n", p.extension().string().c_str());
  return mesh;
}

