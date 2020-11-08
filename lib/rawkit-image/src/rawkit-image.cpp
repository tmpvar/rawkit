#include <rawkit/file.h>
#include <rawkit/hot.h>
#include <rawkit/image.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
using namespace std;

const rawkit_image_t *_rawkit_image_ex(
  const char *from_file,
  const char *path,
  uv_loop_t *loop,
  rawkit_diskwatcher_t *watcher
) {
  string id = string("file+rawkit-image://") + path + " from " + string(from_file);
  rawkit_image_t *image = rawkit_hot_state(id.c_str(), rawkit_image_t);

  const rawkit_file_t* f = _rawkit_file_ex(from_file, path, loop, watcher);

  bool dirty = rawkit_resource_sources(image, f);
  if (!dirty) {
    return image;
  }

  if (f->error) {
    return image;
  }

  // TODO: pass this in via args.
  uint32_t channels = 4;

  int channels_in_file = -1;
  int width = -1;
  int height = -1;
  image->data = (uint8_t *)stbi_load_from_memory(
    f->data,
    f->len,
    &width,
    &height,
    &channels_in_file,
    channels
  );

  image->width = static_cast<uint32_t>(width);
  image->height = static_cast<uint32_t>(height);
  image->len =
    static_cast<uint64_t>(width)  *
    static_cast<uint64_t>(height) *
    static_cast<uint64_t>(channels);

  image->channels = channels;
  image->resource_version++;

  return image;
}