#include <doctest.h>

#include <uv.h>
#include <rawkit/mesh.h>

TEST_CASE("[rawkit/mesh] load missing mesh") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  rawkit_diskwatcher_t* watcher = rawkit_diskwatcher_ex(&loop);
  REQUIRE(watcher != nullptr);
  int s = 1000;

  while (s--) {
    const rawkit_mesh_t* mesh = rawkit_mesh_ex(
      "fixtures/enoent.stl",
      &loop,
      watcher
    );
    REQUIRE(mesh != nullptr);

    if (mesh->resource_version) {
      CHECK(mesh->normal_data == nullptr);
      CHECK(mesh->index_data == nullptr);
      CHECK(mesh->vertex_data == nullptr);

      CHECK(rawkit_mesh_vertex_count(mesh) == 0);
      CHECK(rawkit_mesh_index_count(mesh) == 0);
      CHECK(rawkit_mesh_normal_count(mesh) == 0);
      break;
    }

    uv_run(&loop, UV_RUN_NOWAIT);
  }

  CHECK(s < 0);
}

TEST_CASE("[rawkit/mesh] binary stl") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  rawkit_diskwatcher_t* watcher = rawkit_diskwatcher_ex(&loop);
  REQUIRE(watcher != nullptr);
  int s = 100000;

  while (s--) {
    const rawkit_mesh_t* mesh = rawkit_mesh_ex(
      "fixtures/stl/cube_binary.stl",
      &loop,
      watcher
    );
    REQUIRE(mesh != nullptr);

    if (mesh->resource_version > 0) {
      REQUIRE(mesh->normal_data != nullptr);
      REQUIRE(mesh->vertex_data != nullptr);
      REQUIRE(mesh->index_data != nullptr);
      // TODO: this count can be optimized!
      CHECK(rawkit_mesh_vertex_count(mesh) == 36);
      CHECK(rawkit_mesh_index_count(mesh) == 36);
      CHECK(rawkit_mesh_normal_count(mesh) == 12);
      break;
    }

    uv_run(&loop, UV_RUN_NOWAIT);
  }

  CHECK(s > 0);
}

TEST_CASE("[rawkit/mesh] ascii stl") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  rawkit_diskwatcher_t* watcher = rawkit_diskwatcher_ex(&loop);
  REQUIRE(watcher != nullptr);
  int s = 100000;

  while (s--) {
    const rawkit_mesh_t* mesh = rawkit_mesh_ex(
      "fixtures/stl/cube_ascii.stl",
      &loop,
      watcher
    );
    REQUIRE(mesh != nullptr);

    if (mesh->resource_version > 0) {
      REQUIRE(mesh->normal_data != nullptr);
      REQUIRE(mesh->vertex_data != nullptr);
      REQUIRE(mesh->index_data != nullptr);
      // TODO: this count can be optimized!
      CHECK(rawkit_mesh_vertex_count(mesh) == 36);
      CHECK(rawkit_mesh_index_count(mesh) == 36);
      CHECK(rawkit_mesh_normal_count(mesh) == 12);
      break;
    }

    uv_run(&loop, UV_RUN_NOWAIT);
  }

  CHECK(s > 0);
}