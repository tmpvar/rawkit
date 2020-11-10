#pragma once

#include "stb_sb.h"

#include <string>
#include <sstream>
using namespace std;


#define stl_binary_read_float(dst, data, len, loc) { \
  if (loc > len) { \
    printf("ERROR: corrupted STL (unexpected EOF)\n"); \
    return; \
  } \
  sb_push(dst, *((float *)&data[loc])); \
  loc += 4; \
}

#define stl_binary_read_vec3(dst, data, len, loc) { \
  stl_binary_read_float(dst, data, len, loc); \
  stl_binary_read_float(dst, data, len, loc); \
  stl_binary_read_float(dst, data, len, loc); \
}

static void stl_ascii_init(const rawkit_file_t *file, rawkit_mesh_t *target) {
  rawkit_mesh_t mesh = {0};

  stringstream ascii((const char *)file->data);
  string line;
  uint32_t index = 0;
  // TODO: I was feeling sort of lazy/impatient when I wrote this and it can be improved
  while(getline(ascii, line,'\n')){
    stringstream linestream(line);
    string prefix;
    linestream >> prefix;

    if (prefix == "solid") {
      continue;
    }

    if (prefix == "facet") {
      string nop;
      float x, y, z;
      linestream >> nop >> x >> y >> z;
      sb_push(mesh.normal_data, x);
      sb_push(mesh.normal_data, y);
      sb_push(mesh.normal_data, z);
    }

    if (prefix == "vertex") {
      float x, y, z;
      linestream >> x >> y >> z;
      sb_push(mesh.vertex_data, x);
      sb_push(mesh.vertex_data, y);
      sb_push(mesh.vertex_data, z);
      sb_push(mesh.index_data, index++);
    }

    if (prefix == "endsolid") {
      sb_free(target->index_data);
      sb_free(target->vertex_data);
      sb_free(target->normal_data);

      target->index_data = mesh.index_data;
      target->vertex_data = mesh.vertex_data;
      target->normal_data = mesh.normal_data;

      return;
    }
  }

  sb_free(mesh.index_data);
  sb_free(mesh.vertex_data);
  sb_free(mesh.normal_data);
}

static void stl_binary_init(const rawkit_file_t *file, rawkit_mesh_t *target) {
  rawkit_mesh_t mesh = {};

  const uint8_t *data = file->data;
  const uint32_t len = file->len;
  if (len < 84) {
    printf("ERROR: invalid stl file (no space for header)\n");
    return;
  }
  uint32_t triangle_count = *((uint32_t *)&data[80]);

  uint32_t index = 0;
  uint32_t triangle = 0;
  for (uint32_t loc = 84; loc < len;) {
    // normal
    stl_binary_read_vec3(mesh.normal_data, data, len, loc);

    // triangle data
    {
      stl_binary_read_vec3(mesh.vertex_data, data, len, loc);
      sb_push(mesh.index_data, index++);

      stl_binary_read_vec3(mesh.vertex_data, data, len, loc);
      sb_push(mesh.index_data, index++);

      stl_binary_read_vec3(mesh.vertex_data, data, len, loc);
      sb_push(mesh.index_data, index++);
    }

    // skip attribute
    loc += 2;
  }

  sb_free(target->index_data);
  sb_free(target->vertex_data);
  sb_free(target->normal_data);

  memcpy(target, &mesh, sizeof(mesh));
}
