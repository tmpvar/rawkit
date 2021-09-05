#pragma once

#include <rawkit/hot.h>

#include <unordered_map>


struct rawkit_hot_entry_t {
  const char *name;
  void *data;
  uint64_t len;
};

struct rawkit_hot_context_t {
  std::unordered_map<uint64_t, rawkit_hot_entry_t> state_registry;
};