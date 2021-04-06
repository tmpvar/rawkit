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
  uint64_t diskwatcher_version;
  file_status status;
  ps_t *stream;
  ps_val_t *value;
} file_state_t;

static const rawkit_file_t ERR = {};

const rawkit_file_t *_rawkit_file_ex(const char *from_file, const char *path, uv_loop_t *loop, rawkit_diskwatcher_t *watcher) {
  fs::path rel_dir = fs::path(from_file).remove_filename();

  fs::path full_path(path);
  if (!fs::exists(full_path)) {
    full_path = rel_dir / full_path;
  }

  string id = full_path.string();

  rawkit_file_t *f = rawkit_hot_resource(id.c_str(), rawkit_file_t);

  if (!f) {
    printf("ERROR: _rawkit_file_ex failed to retrieve hot resource (%s)\n", id.c_str());
    return (const rawkit_file_t *)&ERR;
  }

  if (!f->_state) {
    f->_state = (file_state_t *)calloc(sizeof(file_state_t), 1);
    if (!f->_state) {

      printf("ERROR: _rawkit_file_ex failed to allocate file_state_t (%s)\n", id.c_str());
      return (const rawkit_file_t*)&ERR;
    }
  }

  file_state_t *state = (file_state_t *)f->_state;

  if (watcher) {
    uint64_t version = rawkit_diskwatcher_file_version(watcher, full_path.string().c_str());

    if (version != state->diskwatcher_version) {
      state->diskwatcher_version = version;
      state->status = INIT;
    }
  }

  switch (state->status) {
    case INIT: {

      if (!fs::exists(full_path)) {
        f->error = RAWKIT_FILE_NOT_FOUND;
        state->status = NOT_FOUND;
        break;
      }


      state->status = RUNNING;
      ps_destroy(state->stream);
      state->stream = ps_pipeline(
        create_file_source(full_path.string().c_str(), loop),
        create_collector()
      );

      break;
    }

    case NOT_FOUND:
      printf("ERROR: _rawkit_file_ex file could nto be found (%s)\n", full_path.string().c_str());
      return f;

    default: {
      ps_val_t *v = state->stream->fn(state->stream, PS_OK);

      if (v) {
        ps_destroy(state->value);
        state->value = v;
        f->data = (uint8_t *)v->data;
        f->len = v->len;
        state->status = DONE;
        f->resource_version++;
        return f;
      }

      break;
    }

  }

  return f;
}