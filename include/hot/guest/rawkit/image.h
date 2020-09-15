#pragma once

#include <stdint.h>

typedef struct rawkit_image {
  void *pixels;
  float width;
  float height;
  uint8_t channels;
} rawkit_image;

#ifdef __cplusplus
extern "C" {
#endif

  const rawkit_image rawkit_load_image_relative_to_file(const char *from_file, const char *path);
  #define rawkit_load_image(path) rawkit_load_image_relative_to_file(__FILE__, path)

#ifdef __cplusplus
}
#endif