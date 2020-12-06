#include <rawkit/hot.h>
#include <rawkit/core.h>

#include <string.h>

#include <unordered_map>
using namespace std;

typedef struct entry_t {
  const char *name;
  void *data;
  uint64_t len;
} entry_t;

static unordered_map<uint64_t, entry_t> state_registry;

void *_rawkit_hot_state(const char *name, uint64_t id, uint64_t len, void *data, uint8_t is_resource) {
  auto it = state_registry.find(id);
  if (it != state_registry.end()) {
    // TODO: check if len is the same as the entry. When should we memset 0 vs resize?
    return it->second.data;
  }

  entry_t entry = {};
  entry.len = len;
  if (!data) {
    data = calloc(len, 1);
    if (!data) {
      return NULL;
    }
  }

  if (is_resource) {
    rawkit_resource_t* res = (rawkit_resource_t *)data;
    res->resource_id = id;
    res->resource_name = strdup(name);
  }

  entry.data = data;
  state_registry.emplace(id, entry);

  return data;
}

void rawkit_hot_resource_destroy(uint64_t id) {
  auto it = state_registry.find(id);
  if (it == state_registry.end()) {
    return;
  }

  void *entry = it->second.data;
  if (entry) {
    free(entry);
  }

  state_registry.erase(it);
}


rawkit_cpu_buffer_t *rawkit_cpu_buffer(const char *name, uint64_t size) {
  rawkit_cpu_buffer_t *buffer = rawkit_hot_resource(name, rawkit_cpu_buffer_t);
  if (!buffer) {
    return nullptr;
  }

  if (buffer->size == size) {
    return buffer;
  }

  if (!buffer->data) {
    void *data = calloc(size, 1);
    if (!data) {
      printf("ERROR: rawkit_cpu_buffer: could not create buffer\n");
      return buffer;
    }
    buffer->data = data;
  } else {
    void *data = realloc(buffer->data, size);
    if (!data) {
      printf("ERROR: rawkit_cpu_buffer: could not resize buffer\n");
      return nullptr;
    }
    buffer->data = data;
  }

  buffer->size = size;

  buffer->resource_version++;

  return buffer;
}