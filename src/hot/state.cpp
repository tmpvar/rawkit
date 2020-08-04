#include <hot/guest/hot/state.h>
#include <cimgui.h>

#include <string.h>
#include <unordered_map>
using namespace std;
#include <stdio.h>
struct StateEntry {
  size_t size;
  void *value;
};

unordered_map <HotStateID, StateEntry *> storage;

void *hotState(HotStateID id, size_t size, void *default_value) {
  auto it = storage.find(id);
  bool exists = it != storage.end();

  if (exists) {
    if (it->second->size == size) {
      return it->second->value;
    }
    
    void *new_value = malloc(size);
    
    if (it->second->size < size) {
      memcpy(new_value, it->second->value, size);
    } else {
       if (default_value != nullptr) {
        memcpy(new_value, default_value, size);
      } else {
        memset(new_value, 0, size);
      }
    }

    delete it->second->value;
    it->second->value = new_value;
    it->second->size = size;
    return new_value;
  } else {
    void *new_value = malloc(size);

    if (default_value != nullptr) {
      memcpy(new_value, &default_value, size);
    } else {
      memset(new_value, 0, size);
    }

    StateEntry *e = new StateEntry;
    e->size = size;
    e->value = new_value;

    storage.emplace(id, e);
    
    return new_value;
  }
}