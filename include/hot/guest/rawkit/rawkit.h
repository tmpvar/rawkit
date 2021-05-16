#pragma once

// In the event that anyone uses GLM in a guest.
// Eventually I'd like to ship a copy, but I haven't gone there yet.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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
#include <rawkit/window.h>
#include <rawkit/jit.h>

#include <termcolor.h>
#include <stb_sb.h>

// legacy includes
#include "time.h"

#ifdef __cplusplus
#include "string.h"
#include "serial.h"

extern "C" {
#endif

  float rawkit_randf();

#ifdef __cplusplus
}
#endif

