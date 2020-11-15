#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <rawkit/vg.h>

#include "nanovg-binding.h"

#include <string>
using namespace std;

rawkit_vg_t *rawkit_vg(
  rawkit_gpu_t *gpu,
  VkRenderPass render_pass
) {

  rawkit_vg_t *vg = (rawkit_vg_t *)calloc(sizeof(rawkit_vg_t), 1);
  VGState *state = new VGState();


  VKNVGCreateInfo create_info = {0};
  create_info.device = gpu->device;
  create_info.gpu = gpu->physical_device;
  create_info.renderpass = render_pass;

  state->ctx = nvgCreateVk(
    create_info,
    NVG_ANTIALIAS | NVG_STENCIL_STROKES
  );

  vg->_state = (void *)state;
  return vg;
}


NVGpaint rawkit_vg_texture(rawkit_vg_t *vg, rawkit_texture_t *tex) {

  NVGpaint paint = {};



  return paint;
}