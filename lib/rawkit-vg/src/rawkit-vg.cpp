#include <stdlib.h>

#include <rawkit/vulkan.h>
#include <rawkit/vg.h>
#include "nanovg/nanovg_vk.h"

#include "nanovg-binding.h"

#include <string>
using namespace std;

rawkit_vg_t *rawkit_vg(
  VkDevice device,
  VkPhysicalDevice physical_device,
  VkRenderPass render_pass
) {

  rawkit_vg_t *vg = (rawkit_vg_t *)calloc(sizeof(rawkit_vg_t), 1);
  VGState *state = new VGState();


  VKNVGCreateInfo create_info = {0};
  create_info.device = device;
  create_info.gpu = physical_device;
  create_info.renderpass = render_pass;

  state->ctx = nvgCreateVk(
    create_info,
    NVG_ANTIALIAS | NVG_STENCIL_STROKES
  );

  vg->_state = (void *)state;
  return vg;
}
