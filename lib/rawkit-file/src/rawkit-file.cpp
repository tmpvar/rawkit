#include <uv.h>

#include <rawkit/file.h>
#include <rawkit/hot.h>

#include <string>
using namespace std;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

enum file_status {
  NOT_FOUND = -1,
  INIT = 0,
  RUNNING = 1,
};

typedef struct file_state_t {
  uint64_t version;
  file_status status;
  rawkit_file_t file;
  ps_t *stream;
  ps_val_t *value;
} file_state_t;

static const rawkit_file_t OOM = { 0, RAWKIT_FILE_OOM, 0, NULL };

const rawkit_file_t *_rawkit_file_ex(const char *from_file, const char *path, uv_loop_t *loop) {
  fs::path rel_dir = fs::path(from_file).remove_filename();
  string id = string("file://") + path + " from " + rel_dir.string();

  file_state_t *f = rawkit_hot_state(id.c_str(), file_state_t);

  if (!f) {
    return (const rawkit_file_t *)&OOM;
  }

  switch (f->status) {
    case INIT: {
      fs::path p(path);
      if (!fs::exists(path)) {
        p = rel_dir / p;
        if (!fs::exists(p)) {
          f->file.error = RAWKIT_FILE_NOT_FOUND;
          f->status = NOT_FOUND;
          break;
        }
      }

      f->status = RUNNING;
      f->stream = ps_pipeline(
        create_file_source(p.string().c_str(), loop),
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
        return &f->file;
      }

      break;
    }

  }

  return NULL;
}