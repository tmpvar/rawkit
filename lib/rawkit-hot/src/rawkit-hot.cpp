#include <rawkit/hot.h>
#include <rawkit/core.h>

#include <unordered_map>
using namespace std;

typedef struct entry_t {
  void *data;
  uint64_t len;
} entry_t;

static unordered_map<uint64_t, entry_t> state_registry;

void *_rawkit_hot_state(uint64_t id, uint64_t len, void *data, uint8_t is_resource) {
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
  }

  entry.data = data;
  state_registry.emplace(id, entry);

  return data;
}