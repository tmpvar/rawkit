#include <uv.h>

#include <rawkit/file.h>
#include <rawkit/hot.h>
#include <rawkit/diskwatcher.h>

#include <string>
using namespace std;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

enum file_status {
  NOT_FOUND = -1,
  INIT = 0,
  RUNNING = 1,
  DONE = 2,
};

typedef struct file_state_t {
  uint64_t version;
  file_status status;
  rawkit_file_t file;
  ps_t *stream;
  ps_val_t *value;
} file_state_t;

static const rawkit_file_t OOM = { 0, RAWKIT_FILE_OOM, 0, NULL };

const rawkit_file_t *_rawkit_file_ex(const char *from_file, const char *path, uv_loop_t *loop, rawkit_diskwatcher_t *watcher) {
  fs::path rel_dir = fs::path(from_file).remove_filename();
  string id = string("file://") + path + " from " + rel_dir.string();

  file_state_t *f = rawkit_hot_state(id.c_str(), file_state_t);

  if (!f) {
    return (const rawkit_file_t *)&OOM;
  }

  fs::path full_path(path);
  if (!fs::exists(full_path)) {
    full_path = rel_dir / full_path;
  }

  if (watcher) {
    uint64_t version = rawkit_diskwatcher_file_version(watcher, full_path.string().c_str());
    if (version != f->version) {
      ps_destroy(f->value);
      ps_destroy(f->stream);
      f->version = version;
      f->status = INIT;
    }
  }

  switch (f->status) {
    case INIT: {

      if (!fs::exists(full_path)) {
        f->file.error = RAWKIT_FILE_NOT_FOUND;
        f->status = NOT_FOUND;
        break;
      }


      f->status = RUNNING;
      f->stream = ps_pipeline(
        create_file_source(full_path.string().c_str(), loop),
        create_collector()
      );

      break;
    }

    case NOT_FOUND:
      return &f->file;

    default: {
      ps_val_t *v = f->stream->fn(f->stream, PS_OK);

      if (v) {
        ps_destroy(f->value);
        f->value = v;
        f->file.data = (uint8_t *)v->data;
        f->file.len = v->len;
        f->status = DONE;
        return &f->file;
      }

      break;
    }

  }

  return NULL;
}