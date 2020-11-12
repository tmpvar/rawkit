#pragma once

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include <rawkit/time.h>
#include <rawkit/file.h>
#include <rawkit/gpu.h>
#include <rawkit/hot.h>
#include <rawkit/glsl.h>
#include <rawkit/vulkan.h>
#include <rawkit/texture.h>
#include <rawkit/shader.h>
#include <rawkit/vg.h>

// legacy includes
#include "time.h"
#include "legacy-image.h"

#ifdef __cplusplus
#include "string.h"
#include "serial.h"

extern "C" {
#endif

  float rawkit_randf();

  uint32_t rawkit_window_frame_index();
  uint32_t rawkit_window_frame_count();
  uint32_t rawkit_window_width();
  uint32_t rawkit_window_height();

#ifdef __cplusplus
}
#endif

