#include <rawkit/diskwatcher.h>

#include <stdlib.h>

#include <unordered_map>
#include <string>
using namespace std;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

class WatcherEntry {
  public:
    uint64_t version = 0;
    fs::path dir;
    fs::path full_path;

    WatcherEntry(fs::path dir, uint64_t version, fs::path full_path)
    : dir(dir)
    , version(version)
    , full_path(full_path)
    {

    }
};

typedef struct rawkit_diskwatcher_t {
  unordered_map<string, WatcherEntry *> *entries;
  uv_loop_t *loop;
} rawkit_diskwatcher_t;

rawkit_diskwatcher_t *rawkit_default_diskwatcher() {
  static rawkit_diskwatcher_t *default_watcher = NULL;
  if (!default_watcher) {
    default_watcher = _rawkit_diskwatcher_ex(uv_default_loop());
  }

  return default_watcher;
}

void _rawkit_diskwatcher_destroy(rawkit_diskwatcher_t **watcher) {
  if (!watcher) {
    return;
  }

  (*watcher)->entries->clear();
  delete (*watcher)->entries;
  free(*watcher);
  *watcher = NULL;
}

rawkit_diskwatcher_t *_rawkit_diskwatcher_ex(uv_loop_t *loop) {
  rawkit_diskwatcher_t *watcher = (rawkit_diskwatcher_t *)calloc(
    sizeof(rawkit_diskwatcher_t),
    1
  );

  if (!watcher) {
    return NULL;
  }

  watcher->entries = new unordered_map<string, WatcherEntry *>;

  watcher->loop = loop;
  return watcher;
}

void on_file_event(uv_fs_event_t* handle, const char* filename, int events, int status) {
  WatcherEntry *entry = (WatcherEntry *)handle->data;

  if (!entry) {
    printf("rawkit-diskwatcher: error: invalid baton\n");
  }

  if (!filename) {
    return;
  }

  fs::path full = entry->dir / fs::path(filename).filename();
  if (full == entry->full_path) {
    entry->version++;
  }
}

uint64_t rawkit_diskwatcher_file_version(rawkit_diskwatcher_t *watcher, const char *full_path) {
  if (!watcher) {
    return 0;
  }

  string str = full_path;
  auto it = watcher->entries->find(str);
  if (it == watcher->entries->end()) {
    uint64_t version = 0;

    fs::path dir = fs::path(full_path).remove_filename();
    if (!fs::exists(dir)) {
      return 0;
    }

    if (fs::exists(full_path)) {
      version = 1;
    }

    uv_fs_event_t* req = (uv_fs_event_t *)calloc(sizeof(uv_fs_event_t), 1);
    uv_fs_event_init(watcher->loop, req);
    WatcherEntry *entry = new WatcherEntry(dir, version, full_path);
    req->data = (void *)entry;
    watcher->entries->emplace(str, entry);

    uv_fs_event_start(
      req,
      on_file_event,
      dir.string().c_str(),
      UV_FS_EVENT_RECURSIVE
    );
    
    return version;
  }

  return it->second->version;
}
