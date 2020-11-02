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


#ifdef __cplusplus
}
#endif